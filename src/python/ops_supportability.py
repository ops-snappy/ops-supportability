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
import array
import datetime
import fcntl
import filecmp
import glob
import ops_eventlog
import os
import ovs.daemon
import ovs.db.idl
import ovs.dirs
import ovs.poller
import ovs.unixctl
import ovs.unixctl.server
import pyinotify
import subprocess
import sys
import termios
import xattr
from shutil import copyfile
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

# Signal to String mapping

strsignal = ("Unknown signal", "Hangup", "Interrupt", "Quit",
             "Illegal instruction", "Trace/breakpoint trap", "Aborted",
             "Bus error", "Floating point exception", "Killed",
             "User defined signal 1", "Segmentation fault",
             "User defined signal 2", "Broken pipe", "Alarm clock",
             "Terminated", "Stack fault", "Child exited", "Continued",
             "Stopped (signal)", "Stopped", "Stopped (tty input)",
             "Stopped (tty output)", "Urgent I/O condition",
             "CPU time limit exceeded", "File size limit exceeded",
             "Virtual timer expired", "Profiling timer expired",
             "Window changed", "I/O possible", "Power failure",
             "Bad system call")

core_folder = '/var/lib/systemd/coredump/'
processed_files = core_folder + 'processed_core_files.cfl'
core_pattern = core_folder + 'core*'
watchmanager_inst = None

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


# ------------------- post_crash_processing() -----------------
# Performs post crash task
# Currently this function performs the following action
# 1. Send Crash Event Log
#      Read core dump information stored in the extended attributes of the
#      core dump file and send crash event using this information.
def post_crash_processing(corefile):
    try:
        # Name of the Crashed process
        # ToFix :: In case of python based daemon crash, this value just
        #          has the name "python", it doesn't provide information
        #          on which daemon crashed.  This needs to be improved.
        process = xattr.getxattr(corefile, 'user.coredump.comm')

        # Signal Number leading to crash
        signal = int(xattr.getxattr(corefile, 'user.coredump.signal'))
        if signal >= len(strsignal):
            signal = 0

        # Timestamp of the crash event
        timestamp = xattr.getxattr(corefile, 'user.coredump.timestamp')
        timestamp_human = datetime.datetime.fromtimestamp(
            int(timestamp)
            ).strftime('%Y-%m-%d %H:%M:%S')

        ops_eventlog.log_event('SUPPORTABILITY_DAEMON_CRASH',
                               ['process', process],
                               ['signal', strsignal[signal]],
                               ['timestamp', timestamp_human])
    except:
        vlog.dbg("Invalid core dump file found")


# ---------------- process_coredumps() ------------------------------
# Checks whether a new core dump is available.  If found it will
# call post_crash_processing to perform post crash processing

def process_coredumps():
    corefile_list = glob.glob(core_pattern)
    new_core_dump_found = False

    # If the processed file is not available, then create the same
    if os.path.exists(processed_files) is not True:
        with open(processed_files, "w") as f:
            pass

    # Chech whether the coredump files available in the coredump folder
    # are already processed.  In case if a file is not processed, call
    # post_crash_processing over that file.
    with open(processed_files, "r") as f:
        try:
            for corefile in corefile_list:
                core_existing = False
                f.seek(0)
                for line in f:
                    if corefile in line:
                        core_existing = True
                        break
                if core_existing is False:
                    new_core_dump_found = True
                    post_crash_processing(corefile)
        except:
            vlog.info("Hit exception during coredump processing")

    # Update the processed coredump list file
    if new_core_dump_found:
        with open(processed_files, "w") as f:
            f.write(str(corefile_list))


# We are using inotify to watch the core dump folder for new core dumps
# The fd returned by inotify is polled using ovs poller for notification
# ovs poller will return when one of the fd it polls over has become readable
# This function checks whether inotify fd has become readable or not
# If inotify fd is readable then it will call the process_coredumps function
# to process the core dump present
def crashprocessing_run():
    global watchmanager_inst
    watch_fd = watchmanager_inst.get_fd()
    sizebuffer = array.array('i', [0])

    if fcntl.ioctl(watch_fd, termios.FIONREAD, sizebuffer, 1) == -1:
        return

    bytes_available = sizebuffer[0]
    if bytes_available > 0:
        vlog.dbg("Core Dump file available "+str(bytes_available))
        os.read(watch_fd, bytes_available)
        process_coredumps()


# Watches core dump folder for new core dump files
# Uses INotify system call to monitor the core dump folder
# INotify returns a fd which could be polled for notification
# Uses the ovs poller and adds the fd for polling.
# Currently the poller polls over ovsdb, unixctl and inotify
def crashprocessing_poll(poller):
    global watchmanager_inst

    watch_fd = watchmanager_inst.get_fd()
    poller.fd_wait(watch_fd, ovs.poller.POLLIN)


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
    global watchmanager_inst
    schema_helper = ovs.db.idl.SchemaHelper(location=ovs_schema)
    schema_helper.register_columns(SYSTEM_TABLE,
                                   [SYSTEM_SYSLOG_REMOTES_COLUMN])

    schema_helper.register_columns(SYSLOG_REMOTE_TABLE,
                                   [SYSLOG_REMOTE_PROTOCOL_COLUMN,
                                    SYSLOG_REMOTE_REMOTE_COLUMN,
                                    SYSLOG_REMOTE_PORT_COLUMN,
                                    SYSLOG_REMOTE_SEVERTIY_COLUMN])

    idl = ovs.db.idl.Idl(remote, schema_helper)

    ops_eventlog.event_log_init('SUPPORTABILITY')

    watchmanager_inst = pyinotify.WatchManager()

    watchmanager_inst.add_watch(core_folder,
                                pyinotify.IN_MOVED_TO)

    # Check boot time core dumps
    process_coredumps()


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

        crashprocessing_run()

        if exiting:
            break

        if seqno == idl.change_seqno:
            poller = ovs.poller.Poller()
            unixctl_server.wait(poller)
            idl.wait(poller)
            crashprocessing_poll(poller)
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
