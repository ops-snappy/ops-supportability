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

import uuid
from opstestfw import testEnviron, LogOutput


topoDict = {"topoExecution": 120,
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}

# Global variables
dut01Obj = None


def checkShowTechList(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.1 Running Show Tech List Test")
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

    # Run Show Tech List Command
    returnDevInt = dut01Obj.DeviceInteract(command="show tech list")

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
                  "Failed to run Show Tech List " +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if "Show Tech Supported Features List" not in returnDevInt['buffer']:
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech List Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkShowTech(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.2 Running Show Tech Test")
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

    # Run Show Tech Command
    returnDevInt = dut01Obj.DeviceInteract(command="show tech")

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
                  "Failed to run Show Tech" +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if ("Show Tech commands executed successfully"
           not in returnDevInt['buffer']):

            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkShowTechFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.3 Running Show tech Feature Test ")
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

    # Run Show Tech basic Command
    returnDevInt = dut01Obj.DeviceInteract(command="show tech basic")

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
                  "Failed to run Show Tech Basic " +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if ("Show Tech commands executed successfully"
           not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech Feature Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkShowTechSubFeature(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.4 Running Show tech Sub Feature Test ")
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

    # Run Show Tech lldp statistics Command
    returnDevInt = dut01Obj.DeviceInteract(command="show tech lldp statistics")

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
                  "Failed to run Show Tech sub feature " +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if ("Show Tech commands executed successfully"
           not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech SubFeature Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkShowTechToFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.5 Running Show tech to File ")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    outputfile = str(uuid.uuid4()) + ".txt"

    # Get into vtyshell
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    # Run Show Tech Command and store output to file
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech localfile " + outputfile)

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
                  "Failed to run Show Tech to localfile" +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        # Read the file and check the output
        returnDevInt = dut01Obj.DeviceInteract(
            command="cat /tmp/" + outputfile)
        if ("Show Tech commands executed successfully"
           not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech Feature Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkShowTechToFileForce(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.6 Running Show tech to File, Force option ")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    outputfile = str(uuid.uuid4()) + ".txt"

    dut01Obj.DeviceInteract(
        command="touch /tmp/" + outputfile)
    # Get into vtyshell
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    # Run Show Tech Command and store output to file without force
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech localfile " + outputfile)

    if 'already exists' not in returnDevInt['buffer']:
        LogOutput('error', "Force option Failed")
        LogOutput('error', returnDevInt['buffer'])
        dut01Obj.VtyshShell(enter=False)
        return False

    # Run Show Tech Command and store output to file with force
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech localfile " + outputfile + " force")

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
                  "Failed to run Show Tech to localfile with force" +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        # Read the file and check the output
        returnDevInt = dut01Obj.DeviceInteract(
            command="cat /tmp/" + outputfile)
        if ("Show Tech commands executed successfully"
           not in returnDevInt['buffer']):
            LogOutput('error',
                      "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      " Show Tech Feature Ran Successfully on device " +
                      str(dut01Obj.device))
            return True


def checkInvalidCommandFailure(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "2.1 Running Show tech Cli Command Failure")
    LogOutput('info', "############################################\n")
    # Variables
    overallBuffer = []

    # Backup the Default Yaml File
    command = "cp /etc/openswitch/supportability/ops_showtech.yaml \
    /etc/openswitch/supportability/ops_showtech.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', str(returnDevInt['buffer']))

    command = "ls /etc/openswitch/supportability "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', str(returnDevInt['buffer']))

    # Add Test Feature with invalid show command (feature_name: test1234)
    command = "printf '\n  feature:\n  -\n    feature_desc: \"sttest\"\n\
    feature_name: test1234\n    cli_cmds:\n      - \"show testing\"' >> \
     /etc/openswitch/supportability/ops_showtech.yaml"
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', str(returnDevInt['buffer']))

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    # Run Show Tech test1234 Command
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech test1234",
        errorCheck=False,
        CheckError="NOPlease")
    LogOutput('info', str(returnDevInt['buffer']))
    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False
    dut01Obj.DeviceInteract(command="mv \
    /etc/openswitch/supportability/ops_showtech.yaml2 \
    /etc/openswitch/supportability/ops_showtech.yaml")

    overallBuffer.append(returnDevInt['buffer'])

    if ("failed to execute"
            not in returnDevInt['buffer']):
        LogOutput('error',
                  "Test Case Failure,refer output below")
        for outputs in overallBuffer:
            LogOutput('info', str(outputs))
        return False
    else:
        LogOutput('info',
                  "Show Tech Invalid Cli Command Ran Successfully on device" +
                  str(dut01Obj.device))
        return True


def checkShowTechInvalidParameters(dut01Obj):
    LogOutput('info', "\n#################################################")
    LogOutput('info', "2.2 Running Show tech Command with extra Parameter ")
    LogOutput('info', "##################################################\n")
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

    # Run Show Tech lldp statistics Command
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech lldp statistics extraparameter",
        errorCheck=False
    )

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    overallBuffer.append(returnDevInt['buffer'])
    if ("% Unknown command"
       not in returnDevInt['buffer']):
        LogOutput('error',
                  "Test Case Failure,refer output below")
        for outputs in overallBuffer:
            LogOutput('info', str(outputs))
        return False
    else:
        LogOutput('info',
                  " Test Show tech Command with extra Parameter ran \
                  successfully on device " +
                  str(dut01Obj.device))
        return True


def checkShowTechUnSupportedFeature(dut01Obj):
    LogOutput('info',
              "\n#########################################################")
    LogOutput('info',
              "2.3 Running Show tech Command with Unsupported Feature Name ")
    LogOutput('info',
              "############################################################\n")

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

    # Run Show Tech lldp statistics Command
    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech  !@#$%^&*((QWERTYUIOPLFDSAZXCVBNM<>)(&^%$#!",
        errorCheck=False
    )

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    overallBuffer.append(returnDevInt['buffer'])
    if ("Feature !@#$%^&*((QWERTYUIOPLFDSAZXCVBNM<>)(&^%$#! is not supported"
       not in returnDevInt['buffer']):
        LogOutput('error',
                  "Test Case Failure,refer output below")
        for outputs in overallBuffer:
            LogOutput('info', str(outputs))
        return False
    else:
        LogOutput('info',
                  " Test Show tech Command with Unsupported feature name ran \
                  successfully on device " +
                  str(dut01Obj.device))
        return True


def checkShowTechUnSupportedSubFeature(dut01Obj):
    LogOutput('info',
              "\n#########################################################")
    LogOutput(
        'info',
        "2.4 Running Show tech Command with Unsupported Sub Feature Name ")
    LogOutput('info',
              "############################################################\n")

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

    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech lldp !@#$%^&*((QWERTYUIOPLFDSAZXCVBNM<>)(&^%$#!",
        errorCheck=False
    )

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    overallBuffer.append(returnDevInt['buffer'])
    if (
        "Sub Feature !@#$%^&*((QWERTYUIOPLFDSAZXCVBNM<>)"
        "(&^%$#! is not supported"
       not in returnDevInt['buffer']):
        LogOutput('error',
                  "Test Case Failure,refer output below")
        for outputs in overallBuffer:
            LogOutput('info', str(outputs))
        return False
    else:
        LogOutput('info',
                  " Test Show tech Command with Unsupported SubFeature \
                  successfully on device " +
                  str(dut01Obj.device))
        return True


def checkShowTechUnSupportedFeatureAndSubFeature(dut01Obj):
    LogOutput('info',
              "\n#########################################################")
    LogOutput(
        'info',
        "2.5 Running Show tech Command with Unsupported Feature and \
      Sub Feature Name ")
    LogOutput('info',
              "############################################################\n")

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

    returnDevInt = dut01Obj.DeviceInteract(
        command="show tech !@#$%^&*^%$#! !@#$%^&*^%$#!",
        errorCheck=False
    )

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    overallBuffer.append(returnDevInt['buffer'])
    if (
        "Sub Feature !@#$%^&*^%$#! is not supported"
       not in returnDevInt['buffer']):
        LogOutput('error',
                  "Test Case Failure,refer output below")
        for outputs in overallBuffer:
            LogOutput('info', str(outputs))
        return False
    else:
        LogOutput('info',
                  " Test Show tech Command with both Unsupported Feature and \
                  Unsupported SubFeature ran successfully on device " +
                  str(dut01Obj.device))
        return True


def TestNoShowTechConfigfile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "3.1 Running No Show Tech Config File Test")
    LogOutput('info', "############################################\n")
    # Variables
    command = "mv /etc/openswitch/supportability/ops_showtech.yaml\
     /etc/openswitch/supportability/ops_showtech.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', returnDevInt['buffer'])

    command = "ls /etc/openswitch/supportability "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', returnDevInt['buffer'])

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command="show tech list")
    LogOutput('info', returnDevInt['buffer'])

    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command="mv \
    /etc/openswitch/supportability/ops_showtech.yaml2 \
    /etc/openswitch/supportability/ops_showtech.yaml")
    if "Failed to obtain Show Tech configuration" in returnDevInt['buffer']:
        return True
    else:
        LogOutput('error',
                  "Test Case Failure,refer output below")
        LogOutput('info', str(returnDevInt['buffer']))
        return False
    return False


def TestShowTechCorruptedConfigFile(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "3.2 Running Show Tech Corrupted Config File Test")
    LogOutput('info', "############################################\n")
    # Variables
    command = "cp /etc/openswitch/supportability/ops_showtech.yaml\
     /etc/openswitch/supportability/ops_showtech.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', returnDevInt['buffer'])

    command = "printf '\n  feature:\n  -\n    feature_desc:\"sttest\"\n\
    \t\t\tfeature_name:test1234\nasdfjlj$923 - :23;23klj23' >> \
     /etc/openswitch/supportability/ops_showtech.yaml"
    returnDevInt = dut01Obj.DeviceInteract(command=command)

    command = "ls /etc/openswitch/supportability "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', returnDevInt['buffer'])

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command="show tech list")
    LogOutput('info', returnDevInt['buffer'])

    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command="mv \
    /etc/openswitch/supportability/ops_showtech.yaml2 \
    /etc/openswitch/supportability/ops_showtech.yaml")

    if "Failed to obtain Show Tech configuration" in returnDevInt['buffer']:
        return True
    else:
        LogOutput('error',
                  "Test Case Failure,refer output below")
        LogOutput('info', str(returnDevInt['buffer']))
        return False

    return False


def TestShowTechConfigDuplicateEntries(dut01Obj):
    LogOutput('info', "\n############################################")
    LogOutput('info', "3.3 Running Show Tech Duplicated Config Entries Test")
    LogOutput('info', "############################################\n")
    # Variables
    command = "cp /etc/openswitch/supportability/ops_showtech.yaml\
     /etc/openswitch/supportability/ops_showtech.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=command)
    LogOutput('info', returnDevInt['buffer'])

    command = "printf '\n  feature:\n  -\n    feature_desc: \"sttest\"\n\
    feature_name: test1234\n    cli_cmds:\n      - \"show testing\"' >> \
     /etc/openswitch/supportability/ops_showtech.yaml"

    dut01Obj.DeviceInteract(command=command)
    dut01Obj.DeviceInteract(command=command)
    dut01Obj.DeviceInteract(command=command)

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command="show tech basic")

    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command="mv \
    /etc/openswitch/supportability/ops_showtech.yaml2 \
    /etc/openswitch/supportability/ops_showtech.yaml")

    if "Show Tech commands executed successfully" in returnDevInt['buffer']:
        return True
    else:
        LogOutput('error',
                  "Test Case Failure,refer output below")
        LogOutput('info', str(returnDevInt['buffer']))
        return False

    return False


class Test_showtech:

    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_showtech.testObj = testEnviron(
            topoDict=topoDict)
        Test_showtech.topoObj = \
            Test_showtech.testObj.topoObjGet()
        # Global variables
        global dut01Obj
        dut01Obj = cls.topoObj.deviceObjGet(device="dut01")

    # Positive TestCases
    def test_show_tech_list(self):
        assert(checkShowTechList(dut01Obj))

    def test_show_tech(self):
        assert(checkShowTech(dut01Obj))

    def test_show_tech_feature(self):
        assert(checkShowTechFeature(dut01Obj))

    # def test_show_tech_subfeature(self):
    #    assert(checkShowTechSubFeature(dut01Obj))

    def test_show_tech_to_file(self):
        global dut01Obj
        assert(checkShowTechToFile(dut01Obj))

    def test_show_tech_to_file_force(self):
        global dut01Obj
        assert(checkShowTechToFileForce(dut01Obj))

    # Failure Test Cases
    def test_invalid_command_failure(self):
        global dut01Obj
        assert(checkInvalidCommandFailure(dut01Obj))

    def test_invalid_parameter_failure(self):
        global dut01Obj
        assert(checkShowTechInvalidParameters(dut01Obj))

    def test_unsupported_feature(self):
        global dut01Obj
        assert(checkShowTechUnSupportedFeature(dut01Obj))

    # def test_unsupported_subfeature(self):
    #   global dut01Obj
    #    assert(checkShowTechUnSupportedSubFeature(dut01Obj))

    # def test_unsupported_feature_and_subfeature(self):
    #    assert(checkShowTechUnSupportedFeatureAndSubFeature(dut01Obj))

    # Destructive Test Cases
    def test_show_tech_no_config(self):
        global dut01Obj
        assert(TestNoShowTechConfigfile(dut01Obj))

    def test_show_tech_corrupted_config(self):
        global dut01Obj
        assert(TestShowTechCorruptedConfigFile(dut01Obj))

    def test_show_tech_config_with_duplicate_entries(self):
        global dut01Obj
        assert(TestShowTechConfigDuplicateEntries(dut01Obj))

    # Teardown Class
    def teardown_class(cls):
        # Terminate all nodes
        Test_showtech.topoObj.terminate_nodes()
