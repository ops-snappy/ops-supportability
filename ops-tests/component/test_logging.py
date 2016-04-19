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
OpenSwitch Test for syslog.
List of Test Cases
    1. Default syslog configuration
    2. Syslog configuration with severity specified
    3. Syslog configuration with transport specified
    4. Syslog configuration with all options specified.
"""

from pytest import mark
from time import sleep

TOPOLOGY = """

#  +---------+
#  |         |
#  |   sw1   |
#  |         |
#  +---------+

# Nodes
[type=openswitch name="Switch 1"] sw1

"""


# Helper function to verify configuration
def _syslog_conf_no_conf(switch,
                         cmp_str,
                         rmt_host,
                         trnsprt='',
                         svrty=''):

    # Add configuration to the switch
    with switch.libs.vtysh.Configure() as ctx:
        ctx.logging(remote_host=rmt_host,
                    transport=trnsprt,
                    severity=svrty)

    sleep(1)

    # verify that the configuration is reflected in the rsyslog remote file
    rsysconf = switch("cat /etc/rsyslog.remote.conf", shell='bash')

    if cmp_str not in rsysconf:
        assert False

    # Remove configuration from the switch
    with switch.libs.vtysh.Configure() as ctx:
        ctx.no_logging(remote_host=rmt_host,
                       transport=trnsprt,
                       severity=svrty)

    sleep(1)

    # verify that the configuration is reflected in the rsyslog remote file
    rsysconf = switch("cat /etc/rsyslog.remote.conf", shell='bash')

    if cmp_str in rsysconf:
        assert False


@mark.test_id(33000)
def test_logging_default(topology):
    """
    Verify default Syslog configuration
    """

    sw1 = topology.get('sw1')

    assert sw1 is not None

    # Send configuration for syslog
    _syslog_conf_no_conf(switch=sw1,
                         cmp_str='*.debug @10.0.0.8:514',
                         rmt_host='10.0.0.8'
                         )


@mark.test_id(33001)
def test_logging_severity(topology):
    """
    Verify syslog configuration with severity specified
    """
    sw1 = topology.get('sw1')

    assert sw1 is not None

    # Send configuration for syslog
    _syslog_conf_no_conf(switch=sw1,
                         cmp_str='*.err @10.0.0.9:514',
                         rmt_host='10.0.0.9',
                         svrty=' severity err ')


@mark.test_id(33002)
def test_logging_transport(topology):
    """
    Send syslog configuration with transport specified
    """
    sw1 = topology.get('sw1')

    assert sw1 is not None

    # Send configuration for syslog
    _syslog_conf_no_conf(switch=sw1,
                         cmp_str='*.debug @@10.0.0.10:10942',
                         rmt_host='10.0.0.10',
                         trnsprt=' tcp 10942 ')


@mark.test_id(33003)
def test_logging_transport_and_severity(topology):
    """
    Verify syslog configuration with both transport and
    severity specified
    """
    sw1 = topology.get('sw1')

    assert sw1 is not None

    # Send configuration for syslog
    _syslog_conf_no_conf(switch=sw1,
                         cmp_str='*.warning @@10.0.0.11:429',
                         rmt_host='10.0.0.11',
                         trnsprt=' tcp 4292 ',
                         svrty=' severity warn')
