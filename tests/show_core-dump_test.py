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


def setUpNegativeTestCase():
    LogOutput('info', "\n###############################################")
    LogOutput('info', "1 Running Negative Test Cases for Show Core Dump")
    LogOutput('info', "################################################\n")


def noDaemonConfigFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.1 No Daemon Core Config file")
    LogOutput('info', "############################################\n")
    # Rename Existing Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Unable to read daemon core dump config file"
       not in returnData['buffer']):
        return False
    return True


def emptyDaemonConfigFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.2 Empty Daemon Core Config file")
    LogOutput('info', "############################################\n")
    # Rename Existing Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Invalid daemon core dump config file"
       not in returnData['buffer']):
        return False
    return True


def invalidDaemonConfig(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.3 Corrupted Daemon Core Config file")
    LogOutput('info', "############################################\n")
    # Rename Existing Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "  xpcorpath:asf " >  /etc/ops_corefile.conf'
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Invalid daemon core dump config file"
       not in returnData['buffer']):
        return False
    return True


def noKernelConfig(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.4 No Kernel Core Config file")
    LogOutput('info', "############################################\n")
    # Rename Existing Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "corepath=/tmp/core" >  /etc/ops_corefile.conf'
    )
    dut01Obj.DeviceInteract(
        command="mv /etc/kdump.conf /etc/kdump.conf.backup"
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command="mv /etc/kdump.conf.backup /etc/kdump.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Unable to read kernel core dump config file"
       not in returnData['buffer']):
        return False
    return True


def emptyKernelConfigFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.5 Empty Kernel Core Config file")
    LogOutput('info', "############################################\n")

    # Setup Proper Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "corepath=/tmp/core" >  /etc/ops_corefile.conf'
    )
    dut01Obj.DeviceInteract(
        command="mv /etc/kdump.conf /etc/kdump.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/kdump.conf"
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Invalid kernel core dump config file"
       not in returnData['buffer']):
        return False
    return True


def invalidKernelConfig(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.6 Corrupted Kernel Core Config file")
    LogOutput('info', "############################################\n")

    # Setup Proper Daemon Config File
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "corepath=/tmp/core" >  /etc/ops_corefile.conf'
    )
    dut01Obj.DeviceInteract(
        command="mv /etc/kdump.conf /etc/kdump.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/kdump.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "  xpcorath:asf\nasfklasdf:asdfasdf " >  /etc/kdump.conf'
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf.backup /etc/ops_corefile.conf"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Invalid kernel core dump config file"
       not in returnData['buffer']):
        return False
    return True


def invalidInputParam(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.7 Invalid Input Parameter to cli")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump 2314!@#$!@#$jkj"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnData['buffer']))
    if ("Unknown command" not in returnData['buffer']):
        return False
    return True


def setUpPositiveTestCase(dut01Obj):
    LogOutput('info', "\n###############################################")
    LogOutput('info', "2 Running Positive Test Cases for Show Core Dump")
    LogOutput('info', "################################################\n")
    dut01Obj.DeviceInteract(
        command="mkdir -p /var/tmp/core2"
    )
    dut01Obj.DeviceInteract(
        command="mkdir -p /var/tmp/varcore2"
    )

    # Setup Proper Config Files
    dut01Obj.DeviceInteract(
        command="mv /etc/ops_corefile.conf /etc/ops_corefile.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/ops_corefile.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "corepath= /var/tmp/core2" >  /etc/ops_corefile.conf'
    )
    dut01Obj.DeviceInteract(
        command="mv /etc/kdump.conf /etc/kdump.conf.backup"
    )
    dut01Obj.DeviceInteract(
        command="touch /etc/kdump.conf"
    )
    dut01Obj.DeviceInteract(
        command='echo "path  /var/tmp/varcore2" >  /etc/kdump.conf'
    )
    return True


def noCoreDumps(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.1 No Core Dump Present")
    LogOutput('info', "############################################\n")

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))

    LogOutput('info', str(returnData['buffer']))
    if ("No core dumps are present"
       not in returnData['buffer']):
        return False
    return True


def daemonCoreDumpsPresent(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.2 Daemon Core Dumps Present")
    LogOutput('info', "############################################\n")

    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-lldpd/"
    )

    dut01Obj.DeviceInteract(
     command="touch /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )

    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-portd/"
    )
    dut01Obj.DeviceInteract(
     command="touch /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )

    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )

    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Total number of core dumps : 2"
       not in returnData['buffer']):
        return False

    if("ops-lldpd" not in returnData['buffer']):
        return False

    if("ops-portd" not in returnData['buffer']):
        return False

    return True


def kernelCoreDumpsPresent(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.3 Kernel Core Dumps Present")
    LogOutput('info', "############################################\n")

    dut01Obj.DeviceInteract(
     command="touch /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )
    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
     command="rm /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Total number of core dumps : 1"
       not in returnData['buffer']):
        return False

    if("kernel" not in returnData['buffer']):
        return False

    if("2016-01-22 14:12:35" not in returnData['buffer']):
        return False

    return True


def bothCoreDumpsPresent(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.4 Both Core Dumps Present")
    LogOutput('info', "############################################\n")

    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-lldpd/"
    )

    dut01Obj.DeviceInteract(
     command="touch /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )

    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-portd/"
    )
    dut01Obj.DeviceInteract(
     command="touch /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command="touch /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )
    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))
    dut01Obj.DeviceInteract(
     command="rm /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )

    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )

    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )

    LogOutput('info', str(returnData['buffer']))
    if ("Total number of core dumps : 3"
       not in returnData['buffer']):
        return False

    if("ops-lldpd" not in returnData['buffer']):
        return False

    if("ops-portd" not in returnData['buffer']):
        return False

    if("kernel" not in returnData['buffer']):
        return False

    return True


def corruptedCoreDumps(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "3.1 Core Dumps with Corrupted Names")
    LogOutput('info', "############################################\n")
    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-lldpd/"
    )
    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-portd/"
    )
    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-fake1/"
    )
    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-fake2/"
    )
    dut01Obj.DeviceInteract(
     command="mkdir -p /var/tmp/core2/ops-fake3/"
    )
    dut01Obj.DeviceInteract(
     command='mkdir -p /var/tmp/core2/ops-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/'
    )

    dut01Obj.DeviceInteract(
     command="touch /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )
    dut01Obj.DeviceInteract(

     command="touch /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command="touch /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command='touch /var/tmp/core2/ops-fake1/'
     'dontmatchme.$.20161009.080947.core.tar.gz'
     )
    dut01Obj.DeviceInteract(
     command='touch /var/tmp/core2/ops-fake1/'
     'dontmatchme2%@.20161009.080947.core.tar.gz'
    )
    dut01Obj.DeviceInteract(
     command='touch /var/tmp/core2/ops-fake3/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
             'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
             'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.1.20161'
             '009.080947.core.tar.gz'
    )
    dut01Obj.DeviceInteract(
     command='touch /var/tmp/core2/ops-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/'
             'ccccccccccccccccccccccccccccccccc'
             'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc'
             'cccccccccccccccccccccccccccccccccccccccccccccccccccccc.1.20161'
             '009.080947.core.tar.gz'
    )
    # Get Into VtyshShell
    assert (getIntoVtysh(dut01Obj))

    # Run show core-dump
    # We should get error message "Unable to read daemon core dump config file"
    returnData = dut01Obj.DeviceInteract(
        command="show core-dump"
    )

    # Cleanup
    assert(getOutOfVtysh(dut01Obj))

    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-lldpd/"
     "ops-lldpd.1.20160217.100955.core.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command="rm /var/tmp/core2/ops-portd/"
     "ops-portd.1.20151121.040436.core.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command="rm /var/tmp/varcore2/vmcore.20160122.141235.tar.gz"
    )
    dut01Obj.DeviceInteract(
     command='rm /var/tmp/core2/ops-fake1/'
     'dontmatchme.$.20161009.080947.core.tar.gz'
     )
    dut01Obj.DeviceInteract(
     command='rm /var/tmp/core2/ops-fake1/'
     'dontmatchme2%@.20161009.080947.core.tar.gz'
    )
    dut01Obj.DeviceInteract(
     command='rm /var/tmp/core2/ops-fake3/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
             'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb'
             'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.1.20161'
             '009.080947.core.tar.gz'
    )
    dut01Obj.DeviceInteract(
     command='rm /var/tmp/core2/ops-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'
             'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/'
             'ccccccccccccccccccccccccccccccccc'
             'cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc'
             'cccccccccccccccccccccccccccccccccccccccccccccccccccccc.1.20161'
             '009.080947.core.tar.gz'
    )
    LogOutput('info', str(returnData['buffer']))
    if ("Total number of core dumps : 5"
       not in returnData['buffer']):
        return False

    if("ops-lldpd" not in returnData['buffer']):
        return False

    if("ops-portd" not in returnData['buffer']):
        return False

    if("kernel" not in returnData['buffer']):
        return False

    if("dontmatchme" in returnData['buffer']):
        return False

    if("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" not in returnData['buffer']):
        return False

    if("ccccccccccccccccccccccccccccccccccccccccccccccccccccc" not in
       returnData['buffer']):
        return False

    return True


class Test_showcoredump:

    # Global variables
    dut01Obj = None

    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_showcoredump.testObj = testEnviron(
            topoDict=topoDict)
        Test_showcoredump.topoObj = \
            Test_showcoredump.testObj.topoObjGet()
        # Global variables
        global dut01Obj
        dut01Obj = cls.topoObj.deviceObjGet(device="dut01")

    # Positive TestCases

    # Negative TestCases
    def test_setupNegativeTestCase(self):
        setUpNegativeTestCase()

    def test_noDaemonConfigFile(self):
        assert(noDaemonConfigFile(dut01Obj))

    def test_emptyDaemonConfigFile(self):
        assert (emptyDaemonConfigFile(dut01Obj))

    def test_invalidDaemonConfig(self):
        assert (invalidDaemonConfig(dut01Obj))

    def test_noKernelConfig(self):
        assert (noKernelConfig(dut01Obj))

    def test_emptyKernelConfigFile(self):
        assert (emptyKernelConfigFile(dut01Obj))

    def test_invalidKernelConfig(self):
        assert (invalidKernelConfig(dut01Obj))

    def test_invalidInputParam(self):
        assert (invalidInputParam(dut01Obj))

    # Positive TestCases
    def test_setupPostiveTestCase(self):
        setUpPositiveTestCase(dut01Obj)

    def test_noCoreDumps(self):
        assert(noCoreDumps(dut01Obj))

    def test_daemonCoreDumpsPresent(self):
        assert(daemonCoreDumpsPresent(dut01Obj))

    def test_kernelCoreDumpsPresent(self):
        assert(kernelCoreDumpsPresent(dut01Obj))

    def test_bothCoreDumpsPresent(self):
        assert(bothCoreDumpsPresent(dut01Obj))

    # Destructive Test Cases

    def test_corruptedCoreDumps(self):
        assert(corruptedCoreDumps(dut01Obj))

    # Teardown Class
    def teardown_class(cls):
        assert(True)
        # Terminate all nodes
        Test_showcoredump.topoObj.terminate_nodes()
