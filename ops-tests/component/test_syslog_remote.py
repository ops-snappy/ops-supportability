# -*- coding: utf-8 -*-
#
# Copyright (C) 2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""
OpenSwitch Test  to verify syslog messages reception at remove servers.
List of Test Cases
1. Test with four udp servers configured
2. Test with four tcp servers configured
3. Test with two udp servers and two tcp servers configured.

"""

from __future__ import unicode_literals, absolute_import
from __future__ import print_function, division
from time import sleep
from .helpers import wait_until_interface_up
# from ipdb import set_trace

TOPOLOGY = """

#                             +---------+
#                             |         |
#       +--------------------->   sw1  <------------------------+
#       |                     |         |                       |
#       |                     +-^-----^-+                       |
#       |                       |     |                         |
#       |                  +----+     +-------+                 |
#       |                  |                  |                 |
#       |                  |                  |                 |
#  +----v----+        +----v----+        +----v----+       +----v----+
#  |         |        |         |        |         |       |         |
#  |   hs1   |        |   hs2   |        |   hs3   |       |   hs4   |
#  |         |        |         |        |         |       |         |
#  +---------+        +---------+        +---------+       +---------+


# Nodes
[type=openswitch name="Switch 1"] sw1
[type=host image="lepinkainen/ubuntu-python-base:latest" name="Host 1"] hs1
[type=host image="lepinkainen/ubuntu-python-base:latest" name="Host 2"] hs2
[type=host image="lepinkainen/ubuntu-python-base:latest" name="Host 3"] hs3
[type=host image="lepinkainen/ubuntu-python-base:latest" name="Host 4"] hs4

# Links
sw1:1 -- hs1:1
sw1:2 -- hs2:1
sw1:3 -- hs3:1
sw1:4 -- hs4:1
"""


switch_config_status = 0
host_config_status = 0


def _switchconf(sw1, sw_configs):
    """
    Helper function to configure the switch
    It assigns the IP Address for the switch ports connected to the hosts
    Params :
        sw1 :  The switch object
        config : Configuration dictionary to configure switch ports
    """
    global switch_config_status
    if switch_config_status == 0:
        switch_config_status = 1

        for swcfg in sw_configs:
            # Configure IP and bring UP switch  interfaces
            with sw1.libs.vtysh.ConfigInterface(swcfg['int']) as ctx:
                ctx.ip_address(swcfg['ip'])
                ctx.no_shutdown()

        # Wait until interfaces are up
        for swcfg in sw_configs:
            wait_until_interface_up(sw1, swcfg['int'])


def _remote_syslog_test(remotes_config):
    """
    Helper function to perform test based on configuration provided
    """
    global host_config_status
    for conn in remotes_config:

        if(conn['trans'] == 'udp'):
            script = "../../../test//syslog_udp_server.py"
            execscript = "/tmp/syslog_udp_server.py"
        elif(conn['trans'] == 'tcp'):
            script = "../../../test//syslog_tcp_server.py"
            execscript = "/tmp/syslog_tcp_server.py"

        conn['hs']('rm -f /tmp/syslog_out.sb')

        if host_config_status == 0:
            conn['hs'].libs.ip.interface(conn['int'], addr=conn['hs_addr'],
                                         up=True)

        with open(script, "r") as fi:
            for line in fi:
                conn['hs']('echo "' + line + '" >> ' + execscript)

        conn['hs'](
            "python " + execscript + " " +
            conn['rmt_addr'] + " " + conn['port'] + "&"
        )

        with conn['sw'].libs.vtysh.Configure() as ctx:
            ctx.logging(remote_host=conn['rmt_addr'],
                        transport=" " + conn['trans'] + " " + conn["port"])

    host_config_status = 1
    remotes_config[0]['sw']("systemctl restart ops-fand", shell="bash")
    remotes_config[0]['sw']("systemctl restart ops-fand", shell="bash")
    sleep(2)
    # set_trace()
    for conn in remotes_config:
        print(conn['hs']("ls -Shila /tmp"))
        print(conn['hs']("ps -aux"))
        remote_log = conn['hs']("cat /tmp/syslog_out.sb")
        with conn['sw'].libs.vtysh.Configure() as ctx:
            ctx.no_logging(remote_host=conn['rmt_addr'],
                           transport=" " + conn['trans'] + " " + conn["port"])
        conn['hs']("pkill -f " + execscript)
        print(remote_log)
        if "switch systemd" not in remote_log:
            assert False


def test_udp_connection(topology):
    """
    Verifies syslog messages transmission to 4 different udp syslog
    remote servers
    """
    sw1 = topology.get('sw1')
    hs1 = topology.get('hs1')
    hs2 = topology.get('hs2')
    hs3 = topology.get('hs3')
    hs4 = topology.get('hs4')

    assert sw1 is not None
    assert hs1 is not None
    assert hs2 is not None
    assert hs3 is not None
    assert hs4 is not None

    remote_cfg = [
        {
            "hs": hs1,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.10.2/24",
            "rmt_addr": "10.0.10.2",
            "trans": "udp",
            "port": "11514"
        },
        {
            "hs": hs2,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.20.2/24",
            "rmt_addr": "10.0.20.2",
            "trans": "udp",
            "port": "11514"
        },
        {
            "hs": hs3,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.30.2/24",
            "rmt_addr": "10.0.30.2",
            "trans": "udp",
            "port": "11514"
        },
        {
            "hs": hs4,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.40.2/24",
            "rmt_addr": "10.0.40.2",
            "trans": "udp",
            "port": "11514"
        }]
    switch_configs = [
        {
            "int": "1",
            "ip": "10.0.10.1/24"
        },
        {
            "int": "2",
            "ip": "10.0.20.1/24"
        },
        {
            "int": "3",
            "ip": "10.0.30.1/24"
        },
        {
            "int": "4",
            "ip": "10.0.40.1/24"
        }]
    _switchconf(sw1, switch_configs)
    _remote_syslog_test(remote_cfg)


def test_tcp_connection(topology):
    """
    Verifies syslog messages transmission to 4 different tcp syslog
    remote servers
    """
    sw1 = topology.get('sw1')
    hs1 = topology.get('hs1')
    hs2 = topology.get('hs2')
    hs3 = topology.get('hs3')
    hs4 = topology.get('hs4')

    assert sw1 is not None
    assert hs1 is not None
    assert hs2 is not None
    assert hs3 is not None
    assert hs4 is not None

    remote_cfg = [
        {
            "hs": hs1,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.10.2/24",
            "rmt_addr": "10.0.10.2",
            "trans": "tcp",
            "port": "21514"
        },
        {
            "hs": hs2,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.20.2/24",
            "rmt_addr": "10.0.20.2",
            "trans": "tcp",
            "port": "21514"
        },
        {
            "hs": hs3,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.30.2/24",
            "rmt_addr": "10.0.30.2",
            "trans": "tcp",
            "port": "21514"
        },
        {
            "hs": hs4,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.40.2/24",
            "rmt_addr": "10.0.40.2",
            "trans": "tcp",
            "port": "21514"
        }]
    switch_configs = [
        {
            "int": "1",
            "ip": "10.0.10.1/24"
        },
        {
            "int": "2",
            "ip": "10.0.20.1/24"
        },
        {
            "int": "3",
            "ip": "10.0.30.1/24"
        },
        {
            "int": "4",
            "ip": "10.0.40.1/24"
        }]

    _switchconf(sw1, switch_configs)

    _remote_syslog_test(remote_cfg)


def test_tcp_udp_combination(topology):
    """
    Verifies syslog messages transmission to 4 different syslog with
    combination of tcp and upd based servers
    """
    sw1 = topology.get('sw1')
    hs1 = topology.get('hs1')
    hs2 = topology.get('hs2')
    hs3 = topology.get('hs3')
    hs4 = topology.get('hs4')

    assert sw1 is not None
    assert hs1 is not None
    assert hs2 is not None
    assert hs3 is not None
    assert hs4 is not None

    remote_cfg = [
        {
            "hs": hs1,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.10.2/24",
            "rmt_addr": "10.0.10.2",
            "trans": "tcp",
            "port": "21514"
        },
        {
            "hs": hs2,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.20.2/24",
            "rmt_addr": "10.0.20.2",
            "trans": "tcp",
            "port": "21514"
        },
        {
            "hs": hs3,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.30.2/24",
            "rmt_addr": "10.0.30.2",
            "trans": "udp",
            "port": "11514"
        },
        {
            "hs": hs4,
            "sw": sw1,
            "int": "1",
            "hs_addr": "10.0.40.2/24",
            "rmt_addr": "10.0.40.2",
            "trans": "udp",
            "port": "11514"
        }]
    switch_configs = [
        {
            "int": "1",
            "ip": "10.0.10.1/24"
        },
        {
            "int": "2",
            "ip": "10.0.20.1/24"
        },
        {
            "int": "3",
            "ip": "10.0.30.1/24"
        },
        {
            "int": "4",
            "ip": "10.0.40.1/24"
        }]

    _switchconf(sw1, switch_configs)

    _remote_syslog_test(remote_cfg)
