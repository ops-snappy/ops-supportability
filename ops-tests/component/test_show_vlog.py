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
OpenSwitch Test for show vlog related configurations.
List of Test Cases
1.0 Test the show vlog configurations.
1.1 Test the show vlog daemon.
1.2 Test the show vlog severity level.
1.3 Test the show vlog configured daemon.
1.4 Test the show vlog configured feature.
1.5 Test the show vlog configuration list.
1.6 Test the show vlog daemon with severity.
1.7 Test the show vlog severity with daemon.
2.0 Test the show vlog invalid daemon.
2.1 Test the show vlog invalid severity level.
2.2 Test the show vlog invalid sub-command.
"""
from __future__ import unicode_literals, absolute_import
from __future__ import print_function, division
from time import sleep


TOPOLOGY = """
# +-------+
# |       |
# | ops1  |
# |       |
# +-------+

# Nodes
[type=openswitch name="OpenSwitch 1"] ops1

"""


def test_show_vlog_config(topology):
    """
    Test that a show vlog configuration is functional with a OpenSwitch switch.

    Build a topology of one switch and one host and connect the host to the
    switch.
    """
    ops1 = topology.get('ops1')

    assert ops1 is not None

    # vlog Configure
    result = ops1.libs.vtysh.show_vlog_config()

    if result is True:
        print('1.0 show vlog config test passed')
    else:
        print('show vlog config test failed')


def test_show_vlog_daemon(topology):
    """
    Test the show vlog daemon
    """
    ops1 = topology.get('ops1')

    assert ops1 is not None
    # show vlog daemon
    result = ops1.libs.vtysh.show_vlog_daemon('ops-ledd')

    if result is True:
        print('1.1 show vlog daemon test passed')
    else:
        print('show vlog daemon test failed')


def test_show_vlog_severity(topology):
    """
    Test the severity of show vlog
    """
    ops1 = topology.get('ops1')

    assert ops1 is not None
    # show vlog severity
    result = ops1.libs.vtysh.show_vlog_severity('warn')

    if result is True:
        print('1.2 show vlog severity test passed')
    else:
        print('show vlog severity test failed')


def test_show_vlog_config_daemon(topology):
    """
    Test the show vlog config daemon
    """
    ops1 = topology.get('ops1')

    assert ops1 is not None

    # Vlog config daemon
    with ops1.libs.vtysh.Configure() as ctx:
        ctx.vlog_daemon('ops-lldpd',
                        'file',
                        'warn')

    sleep(1)

    result = ops1.libs.vtysh.show_vlog_config_daemon('ops-lldpd')

    if result is True:
        print('1.3 show vlog config daemon test passed')
    else:
        print('show vlog config daemon test failed')


def test_show_vlog_config_feature(topology):
    """
    Test the show vlog config feature
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    # Vlog config feature
    with ops1.libs.vtysh.Configure() as ctx:
        ctx.vlog_feature('lacp',
                         'syslog',
                         'err')
    sleep(1)

    result = ops1.libs.vtysh.show_vlog_config_feature('lacp')

    if result is True:
        print('1.4 show vlog config feature test passed')
    else:
        print('show vlog config feature test failed')


def test_show_vlog_config_list(topology):
    """
    Test the show vlog config list
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None
    # show vlog config list
    result = ops1.libs.vtysh.show_vlog_config_list()

    if result is True:
        print('1.5 show vlog config list passed')
    else:
        print('show vlog config list failed')


def test_show_vlog_invalid_daemon(topology):
    """
    Test the show vlog daemon passing invalid daemon as argument
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    result = ops1.libs.vtysh.show_vlog_daemon('asjsajsjsa')

    if result is False:
        print('2.0 show vlog invalid daemon test passed')
    else:
        print('show vlog invalid daemon test failed')


def test_show_vlog_invalid_severity_level(topology):
    """
    Test the show vlog severity level passing invalid severity as \
     argument
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    result = ops1.libs.vtysh.show_vlog_severity('akakssalas')

    if result is False:
        print('2.1 show vlog invalid severity test passed')
    else:
        print('show vlog invalid severity test failed')


def test_show_vlog_daemon_severity(topology):
    """
    Test the show vlog daemon {daemon} severity {severity}
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    result = ops1.libs.vtysh.show_vlog_daemon_severity('ops-portd', 'info')

    if result is True:
        print('1.6 show vlog daemon with severity test passed')
    else:
        print('show vlog daemon with severity test failed')


def test_show_vlog_severity_daemon(topology):
    """
    Test the show vlog severity {severity} daemon {daemon}
    """
    ops1 = topology.get('ops1')

    assert ops1 is not None

    result = ops1.libs.vtysh.show_vlog_severity_daemon('info', 'ops-portd')

    if result is True:
        print('1.7 show vlog severity level with daemon test passed')
    else:
        print('show vlog severity with daemon test failed')


def test_show_vlog_subcommand(topology):
    """
    Test the show vlog subcommand
    """

    ops1 = topology.get('ops1')

    assert ops1 is not None

    result = ops1.libs.vtysh.show_vlog('sgsjasgj')

    if result is True:
        print('2.2 show vlog invalid sub-command test passed')
    else:
        print('show vlog invalid subcommand test failed')
