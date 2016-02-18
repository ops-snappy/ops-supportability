# (C) Copyright 2016 Hewlett Packard Enterprise Development LP
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
#

from opstestfw import testEnviron, LogOutput
import re

topoDict = {"topoExecution": 120,
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}


def getIntoVtysh(dut01Obj):
    returnStructure = dut01Obj.VtyshShell(enter=True)
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        return False
    return True


def getOutOfVtysh(dut01Obj):
    returnStructure = dut01Obj.VtyshShell(enter=False)
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get out of vtysh prompt")
        return False
    return True


def getIntoConfig(dut01Obj):
    returnStructure = dut01Obj.ConfigVtyShell(enter=True)
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to enter config terminal")
        return False
    return True


def getOutOfConfig(dut01Obj):
    returnStructure = dut01Obj.ConfigVtyShell(enter=False)
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit config terminal")
        return False
    return True


def checkShowVlogList(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.1 Running show-vlog config list test        ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(command="show vlog config list")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error', "show vlog list failed")
        return False
    else:
        if "Features" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog list failed")
            return False
        if "Description" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog list failed")
            return False
        if "lldp" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog list failed")
            return False
    return True


def checkShowVlogFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.2 Running show vlog feature test            ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(command="show vlog config feature lldp")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error', "show vlog feature failed")
        return False
    else:
        if "Feature" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog feature failed")
            return False
        if "Syslog" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog feature failed")
            return False
        if "lldp" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog feature failed")
            return False
    return True


def checkShowVlog(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.3 Running show vlog config test             ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(command="show vlog config")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error', "show vlog failed")
        return False
    else:
        if "Feature" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog failed")
            return False
        if "Daemon" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog failed")
            return False
        if "Syslog" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog failed")
            return False
        if "lldp" not in returnDevInt['buffer']:
            LogOutput('error', "show vlog failed")
            return False
    return True


def checkVlogConfigFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.4 Check basic Vlog Configuration Feature    ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lacp syslog info")

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lacp file dbg")
    # Clean Up
    assert (getOutOfConfig(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', str(returnDevInt['buffer']))
    if finalReturnCode != 0:
        LogOutput('error', "vlog configuration failed")
        return False

    returnDevInt = dut01Obj.DeviceInteract(
     command="show vlog config feature lacp")
    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))
    regex = re.compile(r"lacp\s+INFO\s+DBG")

    if re.search(regex, returnDevInt['buffer']) is None:
        LogOutput('error', "vlog configuration has not effect : failed")
        return False

    # Now change the setting to DBG and check whether it changes
    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lacp syslog dbg")

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lacp file info")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', str(returnDevInt['buffer']))
    if finalReturnCode != 0:
        LogOutput('error', "vlog configuration failed")
        return False

    returnDevInt = dut01Obj.DeviceInteract(
     command="show vlog config feature lacp")

    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))
    regex = re.compile(r"lacp\s+DBG\s+INFO")

    if re.search(regex, returnDevInt['buffer']) is None:
        LogOutput('error', "vlog configuration has not effect : failed")
        return False
    return True


def checkVlogConfigDaemon(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.5 Check basic Vlog Configuration for Daemon ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog daemon ops-lacpd syslog info")

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog daemon ops-lacpd file dbg")
    # Clean Up
    assert (getOutOfConfig(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', str(returnDevInt['buffer']))
    if finalReturnCode != 0:
        LogOutput('error', "vlog configuration failed")
        return False

    returnDevInt = dut01Obj.DeviceInteract(
     command="show vlog config daemon ops-lacpd")

    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))
    regex = re.compile(r"ops-lacpd\s+INFO\s+DBG")

    if re.search(regex, returnDevInt['buffer']) is None:
        LogOutput('error', "vlog configuration has not effect : failed")
        return False

    # Now change the setting to DBG and check whether it changes
    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog daemon ops-lacpd syslog dbg")

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog daemon ops-lacpd file info")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))

    finalReturnCode = returnDevInt['returnCode']
    LogOutput('info', str(returnDevInt['buffer']))
    if finalReturnCode != 0:
        LogOutput('error', "vlog configuration failed")
        return False

    returnDevInt = dut01Obj.DeviceInteract(
     command="show vlog config daemon ops-lacpd")

    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))
    regex = re.compile(r"ops-lacpd\s+DBG\s+INFO")

    if re.search(regex, returnDevInt['buffer']) is None:
        LogOutput('error', "vlog configuration has not effect : failed")
        return False
    return True


def checkInvalidDaemon(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.1 Run Show vlog for invalid daemon          ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
# simply@#$*beland
    returnDevInt = dut01Obj.DeviceInteract(
        command="show vlog config daemon adsf@f$*ASDfjaklsdf@#Q@3r")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))
    LogOutput('info', str(returnDevInt['returnCode']))
    LogOutput('info', str(returnDevInt['buffer']))

    if ("Not able to communicate with daemon adsf@f$*ASDfjaklsdf@#Q@3r"
       not in returnDevInt['buffer']):
        LogOutput('error', "show vlog for invalid daemon failed")
        return False
    return True


def checkInvalidFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.2 Run Show vlog for invalid feature          ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
        command="show vlog config feature adsf@f$*ASDfjaklsdf@#Q@3r")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))

    if ("Feature not present"
       not in returnDevInt['buffer']):
        LogOutput('error', "show vlog for invalid feature failed")
        return False
    return True


def checkInvalidSubCommand(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.3 Run Show vlog invalid subcommand          ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
        command="show vlog config adsf@f$*ASDfjaklsdf@#Q@3r")

    # Clean Up
    assert (getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnDevInt['buffer']))

    if ("Unknown command" not in returnDevInt['buffer']):
        LogOutput('error', "show vlog for invalid subcommand failed")
        return False
    return True


def checkVlogConfigInvalidFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.4 Check Vlog Configuration for Invaild Feature ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature adsf@f$*ASDfjaklsdf@#Q@3r syslog info")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))
    assert (getOutOfVtysh(dut01Obj))

    if ("Feature not present"
       not in returnDevInt['buffer']):
        LogOutput('error', "config vlog for invalid feature failed")
        return False
    return True


def checkVlogConfigInvalidDaemon(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.5 Check Vlog Configuration for Invaild Daemon ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog daemon adsf@f$*ASDfjaklsdf@#Q@3r syslog info")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))
    assert (getOutOfVtysh(dut01Obj))

    if ("Not able to communicate with daemon adsf@f$*ASDfjaklsdf@#Q@3r"
       not in returnDevInt['buffer']):
        LogOutput('error', "config vlog for invalid daemon failed")
        return False
    return True


def checkVlogConfigInvalidDestination(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.6 Check Vlog Configuration for Invaild Destination ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lldpd adsf@f$*ASDfjaklsdf@#Q@3r info")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))
    assert (getOutOfVtysh(dut01Obj))

    if ("Unknown command" not in returnDevInt['buffer']):
        LogOutput('error', "config vlog for invalid destination failed")
        return False
    return True


def checkVlogConfigInvalidLogLevel(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.7 Check Vlog Configuration for Invaild Loglevel ")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))
    assert (getIntoConfig(dut01Obj))

    returnDevInt = dut01Obj.DeviceInteract(
     command="vlog feature lldpd syslog adsf@f$*ASDfjaklsdf@#Q@3r")

    # Clean Up
    assert (getOutOfConfig(dut01Obj))
    assert (getOutOfVtysh(dut01Obj))

    if ("Unknown command" not in returnDevInt['buffer']):
        LogOutput('error', "config vlog for invalid loglevel failed")
        return False
    return True


class Test_show_vlog:

    # Global variables
    dut01Obj = None

    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_show_vlog.testObj = testEnviron(
            topoDict=topoDict)
        Test_show_vlog.topoObj = \
            Test_show_vlog.testObj.topoObjGet()
        # Global variables
        global dut01Obj
        dut01Obj = cls.topoObj.deviceObjGet(device="dut01")

    # Positive Test Cases
    def test_checkShowVlogList(self):
        assert(checkShowVlogList(dut01Obj))

    def test_checkShowVlogFeature(self):
        assert(checkShowVlogFeature(dut01Obj))

    def test_checkShowVlog(self):
        assert(checkShowVlog(dut01Obj))

    def test_checkVlogConfigFeature(self):
        assert(checkVlogConfigFeature(dut01Obj))

    def test_checkVlogConfigDaemon(self):
        assert(checkVlogConfigDaemon(dut01Obj))

    # Negative Test Cases
    def test_checkInvalidDaemon(self):
        assert(checkInvalidDaemon(dut01Obj))

    def test_checkInvalidFeature(self):
        assert(checkInvalidFeature(dut01Obj))

    def test_checkInvalidSubCommand(self):
        assert(checkInvalidSubCommand(dut01Obj))

    def test_checkVlogConfigInvalidLogLevel(self):
        assert(checkVlogConfigInvalidLogLevel(dut01Obj))

    def test_checkVlogConfigInvalidDestination(self):
        assert(checkVlogConfigInvalidDestination(dut01Obj))

    def test_checkVlogConfigInvalidDaemon(self):
        assert(checkVlogConfigInvalidDaemon(dut01Obj))

    def test_checkVlogConfigInvalidFeature(self):
        assert(checkVlogConfigInvalidFeature(dut01Obj))

    # Teardown Class
    def teardown_class(cls):
        assert(True)
        # Terminate all nodes
        Test_show_vlog.topoObj.terminate_nodes()
