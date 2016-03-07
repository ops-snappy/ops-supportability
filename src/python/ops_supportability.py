#!/usr/bin/env python
# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.

# Common Daemon for Supportability activities.
# Currently this Daemon takes care of the following features
# 1. Syslog configuration update
#       Updates the syslog configuration file based on the values stored in
#       the ovsdb.
#      Its responsibilities include:
#       - Read the ovsdb configuration for syslog during init and generate
#         the syslog configuration file.
#       - Monitor changes in syslog table and regenerate the syslog
#         configuration file as needed
#       - Restart the rsyslog daemon whenever the configuration changes.

import argparse
import sys
import filecmp
from shutil import copyfile
import ovs.dirs
import ovs.daemon
import ovs.db.idl
import ovs.unixctl
import ovs.unixctl.server
import os
import subprocess
from string import Template

# OVS definitions.
idl = None

# Tables definitions.
SYSTEM_TABLE = 'System'
SYSLOG_REMOTE_TABLE = 'Syslog_Remote'

# Columns definitions.
SYSTEM_SYSLOG_REMOTES_COLUMN = 'syslog_remotes'

SYSLOG_REMOTE_PROTOCOL_COLUMN = 'transport'
SYSLOG_REMOTE_REMOTE_COLUMN = 'remote_host'
SYSLOG_REMOTE_PORT_COLUMN = 'port_number'
SYSLOG_REMOTE_SEVERTIY_COLUMN = 'severity'

# Default DB path.
def_db = 'unix:/var/run/openvswitch/db.sock'

# TODO: Need to pull these from the build env.
ovs_schema = '/usr/share/openvswitch/vswitch.ovsschema'

vlog = ovs.vlog.Vlog("ops-supportabilityd")

# Globals
exiting = False
seqno = 0
syslog_transport = ''
syslog_remote = ''
syslog_port_number = 0
syslog_severity = 'debug'


def unixctl_exit(conn, unused_argv, unused_aux):
    global exiting
    exiting = True
    conn.reply(None)


# ---------------- supportability_run_command() ------------
def supportability_run_command(command):
    '''
       This function runs the command provided through 'command'
       and returns the error and output info
    '''
    process = subprocess.Popen(args=command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE,
                               shell=True)
    error = process.stderr.read()
    output = process.communicate()
    return error, output


# ------------------ terminate() ----------------
def terminate():
    global exiting
    # Exiting Daemon.
    exiting = True


# ------------------ supportability_init() ----------------
def supportability_init(remote):
    '''
    Initializes the OVS-DB connection
    '''

    global idl

    schema_helper = ovs.db.idl.SchemaHelper(location=ovs_schema)
    schema_helper.register_columns(SYSTEM_TABLE,
                                   [SYSTEM_SYSLOG_REMOTES_COLUMN])

    schema_helper.register_columns(SYSLOG_REMOTE_TABLE,
                                   [SYSLOG_REMOTE_PROTOCOL_COLUMN,
                                    SYSLOG_REMOTE_REMOTE_COLUMN,
                                    SYSLOG_REMOTE_PORT_COLUMN,
                                    SYSLOG_REMOTE_SEVERTIY_COLUMN])

    idl = ovs.db.idl.Idl(remote, schema_helper)


# ------------------ supportability_reconfigure() ----------------
def supportability_reconfigure():
    # Check if any of the monitored parameter has changed,
    # if changed then update the configuration
    global syslog_transport
    global syslog_remote
    global syslog_port_number
    global syslog_severity

    vlog.info("supportability reconfiguration called")

    sysserver_tmpl_udp = Template('*.$severity @$remote:$port\n')
    sysserver_tmpl_tcp = Template('*.$severity @@$remote:$port\n')

    syslogfile = open("/tmp/rsyslog.remote.conf", "w")

    for syslog_row in idl.tables[SYSLOG_REMOTE_TABLE].rows.itervalues():
        if syslog_row is not None:
            # Set Default Severity
            if len(syslog_row.severity) == 0:
                syslog_severity = "debug"
            else:
                syslog_severity = syslog_row.severity[0]

            # Set Default Tranport Method
            if len(syslog_row.transport) == 0:
                syslog_transport = "udp"
            else:
                syslog_transport = syslog_row.transport[0]

            # Set Default Port
            if syslog_transport == "udp":
                if len(syslog_row.port_number) > 0:
                    syslog_port_number = syslog_row.port_number[0]
                else:
                    syslog_port_number = 514

                syslogfile.write(sysserver_tmpl_udp.safe_substitute(
                                 severity=syslog_severity,
                                 remote=syslog_row.remote_host,
                                 port=syslog_port_number
                                 ))
            elif syslog_transport == "tcp":
                if len(syslog_row.port_number) > 0:
                    syslog_port_number = syslog_row.port_number[0]
                else:
                    syslog_port_number = 1470

                syslogfile.write(sysserver_tmpl_tcp.safe_substitute(
                                 severity=syslog_severity,
                                 remote=syslog_row.remote_host,
                                 port=syslog_port_number
                                 ))
    syslogfile.close()

    # Compare the syslog temporary file with the syslog file to check if any
    # update has happened.  If the files are different then copy the
    # /tmp/rsyslog.remote.conf to /etc/rsyslog.remote.conf and restart the
    # rsyslog server.

    # if the file is not existing then just copy
    if(
        (os.path.isfile("/etc/rsyslog.remote.conf") is not True) or
        (
            filecmp.cmp(
            "/tmp/rsyslog.remote.conf", "/etc/rsyslog.remote.conf"
            ) is not True
        )
    ):
            vlog.info("Syslog remote configuration updated")
            copyfile("/tmp/rsyslog.remote.conf", "/etc/rsyslog.remote.conf")
            supportability_run_command('systemctl restart rsyslog')

    os.remove("/tmp/rsyslog.remote.conf")


# ------------------ supportability_run() ----------------
def supportability_run():

    global idl
    global seqno

    idl.run()

    if seqno != idl.change_seqno:
        supportability_reconfigure()
        seqno = idl.change_seqno


# ------------------ supportability_wait() ----------------
def supportability_wait():
    pass


# ------------------ main() ----------------
def main():

    global exiting
    global idl
    global seqno

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--database', metavar="DATABASE",
                        help="A socket on which ovsdb-server is listening.",
                        dest='database')

    ovs.vlog.add_args(parser)
    ovs.daemon.add_args(parser)
    args = parser.parse_args()
    ovs.vlog.handle_args(args)
    ovs.daemon.handle_args(args)

    if args.database is None:
        remote = def_db
    else:
        remote = args.database

    supportability_init(remote)

    ovs.daemon.daemonize()

    ovs.unixctl.command_register("exit", "", 0, 0, unixctl_exit, None)
    error, unixctl_server = ovs.unixctl.server.UnixctlServer.create(None)

    if error:
        ovs.util.ovs_fatal(error, "could not create unix-ctl server", vlog)

    # Sequence number when we last processed the db.
    seqno = idl.change_seqno

    exiting = False
    while not exiting:

        supportability_run()

        unixctl_server.run()

        supportability_wait()

        if exiting:
            break

        if seqno == idl.change_seqno:
            poller = ovs.poller.Poller()
            unixctl_server.wait(poller)
            idl.wait(poller)
            poller.block()

    # Daemon Exit.
    unixctl_server.close()
    idl.close()


if __name__ == '__main__':
    try:
        main()
    except SystemExit:
        # Let system.exit() calls complete normally.
        raise
    except:
        vlog.exception("traceback")
        sys.exit(ovs.daemon.RESTART_EXIT_CODE)
