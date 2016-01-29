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


def checkDiagDumpList(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.1 Running diag-dump list test               ")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []
    finalReturnCode = 0

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    returnDevInt = dut01Obj.DeviceInteract(command="diag-dump list")

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run diag-dump list" +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if ("Diagnostic Dump Supported Features List"
                not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Diagnostic dump list ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.2 Running diag-dump lldp basic              ")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []
    finalReturnCode = 0

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    returnDevInt = dut01Obj.DeviceInteract(command="diag-dump lldp basic")

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run diag-dump list" +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if ("Diagnostic dump captured for feature"
                not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Diagnostic dump list ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeatureFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.3 Running diag-dump <feature> [file]    test")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    # Run diag-dump lldp basic <file> Command
    returnDevInt = dut01Obj.DeviceInteract(
                   command="diag-dump lldp basic abc.tmp")

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    LogOutput('info', str(returnDevInt['buffer']))

    if "Diagnostic dump captured for feature" in returnDevInt['buffer']:
        return True
    else:
        LogOutput('error',
                  "Test Case Failure,refer output below")
        LogOutput('info', str(returnDevInt['buffer']))
        return False
    return False


class Test_diag_dump:

    # Global variables
    dut01Obj = None

    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_diag_dump.testObj = testEnviron(
            topoDict=topoDict)
        Test_diag_dump.topoObj = \
            Test_diag_dump.testObj.topoObjGet()
        # Global variables
        global dut01Obj
        dut01Obj = cls.topoObj.deviceObjGet(device="dut01")

    def test_diag_dump_list(self):
        assert(checkDiagDumpList(dut01Obj))

    def test_diag_dump_feature(self):
        assert(checkDiagDumpFeature(dut01Obj))

    def test_diag_dump_feature_file(self):
        assert(checkDiagDumpFeatureFile(dut01Obj))
