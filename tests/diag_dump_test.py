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

# Global variables
dut01Obj = None


def checkDiagDumpList(dut01Obj):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Diagnostic Dump Supported Features List'
    vtysh_cmd = 'diag-dump list'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', '1.1 Running' + tc_desc)
    LogOutput('info', "############################################\n")

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
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

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeature(dut01Obj, feature):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Diagnostic dump captured for feature'
    vtysh_cmd = 'diag-dump ' + feature + ' basic'
    tc_desc = vtysh_cmd + ' test '
    LogOutput('info', "\n############################################")
    LogOutput('info', "1.2 Running" + tc_desc)
    LogOutput('info', "############################################\n")

    # Get into vtyshelll
    returnStructure = dut01Obj.VtyshShell(enter=True)
    overallBuffer.append(returnStructure.buffer())
    returnCode = returnStructure.returnCode()
    if returnCode != 0:
        LogOutput('error', "Failed to get vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info', str(curLine))
        return False

    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
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

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput('error',
                      tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeatureFile(dut01Obj, feature, file_txt):
    # Variables
    overallBuffer = []
    finalReturnCode = 0

    str_check = 'PASS'
    str_check_file = "\[Start\] Feature"
    vtysh_cmd = 'diag-dump ' + feature + ' basic ' + file_txt
    tc_desc = vtysh_cmd + ' test '
    diag_file_path = '/tmp/ops-diag/' + file_txt

    LogOutput('info', "\n############################################")
    LogOutput('info', "1.3 Running" + tc_desc)
    LogOutput('info', "############################################\n")

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
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    # exit the vtysh shell
    returnStructure = dut01Obj.VtyshShell(enter=False)
    LogOutput('info', str(returnDevInt['buffer']))

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    shell_cmd = ' if [[ -f ' + diag_file_path \
        + ' ]]; then if [[ $(grep ' + str_check_file + ' ' + diag_file_path  \
        + ' ) -eq 0 ]]  ; then echo  PASS ; else echo FAIL; fi; '\
        + 'else echo FAIL; fi'

    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    overallBuffer.append(returnDevInt['buffer'])

    shell_cmd = ' rm -f ' + diag_file_path
    returnDevIntClean = dut01Obj.DeviceInteract(command=shell_cmd)
    overallBuffer.append(returnDevIntClean['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeatureFilePath(dut01Obj, feature, diag_file):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    vtysh_cmd = 'diag-dump ' + feature + ' basic ' + diag_file
    tc_desc = vtysh_cmd + ' test '
    diag_file_path = '/tmp/ops-diag/' + diag_file
    str_check = diag_file_path

    LogOutput('info', "\n############################################")
    LogOutput('info', '1.4 Running ' + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = 'rm -f ' + diag_file_path
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    returnStructure = dut01Obj.VtyshShell(enter=True)
    # Run diag-dump lldp basic <file> Command
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = 'ls ' + diag_file_path
    returnStructure = dut01Obj.VtyshShell(enter=False)
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    shell_cmd = 'rm -f ' + diag_file_path
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkDiagDumpFeatureFileSize(dut01Obj, feature, diag_file):
    # Variables
    overallBuffer = []
    finalReturnCode = 0

    vtysh_cmd = 'diag-dump ' + feature + ' basic ' + diag_file
    tc_desc = vtysh_cmd + ' size test '
    diag_file_path = '/tmp/ops-diag/' + diag_file
    str_check = 'PASS'

    LogOutput('info', "\n############################################")
    LogOutput('info', '1.5 Running ' + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = 'rm -f ' + diag_file_path
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    returnStructure = dut01Obj.VtyshShell(enter=True)
    # Run diag-dump lldp basic <file> Command
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))
    # exit the vtysh shell

    shell_cmd = ' if [[ -f ' + diag_file_path \
        + ' ]]; then if [[ $(stat -c %s ' + diag_file_path  \
        + ' ) -gt 1 ]]  ; then echo  PASS ; else echo FAIL; fi; '\
        + 'else echo FAIL; fi'

    returnStructure = dut01Obj.VtyshShell(enter=False)
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    overallBuffer.append(returnStructure.buffer())

    # returnDevInt['buffer']
    # 1st line contains script
    # 2nd line contains output result
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    # returnStructure = dut01Obj.VtyshShell(enter=False)
    shell_cmd = 'rm -f ' + diag_file_path
    dut01Obj.DeviceInteract(command=shell_cmd)

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkUnsupportedDaemon(dut01Obj, daemon):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Diagnostic dump ' + daemon + ' feature failed for'
    vtysh_cmd = 'diag-dump ' + daemon + ' basic'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', "2.1 Running" + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = "cp /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = 'printf \'\n  -\n    feature_name: \"' + daemon + \
        '\"\n    feature_desc: \"BGP feature Sample\"\n' + \
        '    daemon:\n     - \"' + daemon + '\"    ' + ' \' ' + \
        ' >> /etc/openswitch/supportability/ops_featuremapping.yaml'

    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=False)

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    shell_cmd = "mv /etc/openswitch/supportability/ops_featuremapping.yaml2\
     /etc/openswitch/supportability/ops_featuremapping.yaml "
    dut01Obj.DeviceInteract(command=shell_cmd)

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkUnknownDaemon(dut01Obj):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'failed to connect'
    vtysh_cmd = 'diag-dump garbage basic'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', '2.2 Running ' + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = "cp /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "printf '\n  -\n    feature_name: \"garbage\"\n\
    feature_desc: \"Dummy feature Sample\"\n\
    daemon:\n     - \"ops-garb-abcd\"' >> \
     /etc/openswitch/supportability/ops_featuremapping.yaml"

    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "mv \
    /etc/openswitch/supportability/ops_featuremapping.yaml2 \
    /etc/openswitch/supportability/ops_featuremapping.yaml"
    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkUnknownGarbDaemon(dut01Obj):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'feature is not present'
    vtysh_cmd = 'diag-dump ops-garb-abcd basic'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', '2.3 Running ' + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = "cp /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "printf '\n  -\n    feature_name: \"garbage\"\n\
    feature_desc: \"Dummy feature Sample\"\n\
    daemon:\n     - \"ops-garb-abcd\"' >> \
     /etc/openswitch/supportability/ops_featuremapping.yaml"

    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "mv \
    /etc/openswitch/supportability/ops_featuremapping.yaml2 \
    /etc/openswitch/supportability/ops_featuremapping.yaml"
    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkNoConfigfile(dut01Obj):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Error in retrieving the mapping of feature names to daemon'
    vtysh_cmd = 'diag-dump lldp basic'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', "3.1 Running " + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = "mv /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "ls /etc/openswitch/supportability "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "mv \
    /etc/openswitch/supportability/ops_featuremapping.yaml2 \
    /etc/openswitch/supportability/ops_featuremapping.yaml"
    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkEmptyFile(dut01Obj):

    # Variables
    vtysh_cmd = 'diag-dump list'
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Error in retrieving the mapping of feature names to daemon'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', "3.2 Running diag-dump empty config file test")
    LogOutput('info', "############################################\n")
    shell_cmd = "mv -f /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "touch /etc/openswitch/supportability/ops_featuremapping.yaml"
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = "mv -f\
    /etc/openswitch/supportability/ops_featuremapping.yaml2 \
    /etc/openswitch/supportability/ops_featuremapping.yaml"
    dut01Obj.VtyshShell(enter=False)
    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])
    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


def checkCorruptedYamlFile(dut01Obj):
    # Variables
    overallBuffer = []
    finalReturnCode = 0
    str_check = 'Diagnostic dump captured for feature'
    daemon = 'lldp'
    vtysh_cmd = 'diag-dump ' + daemon + ' basic'
    tc_desc = vtysh_cmd + ' test '

    LogOutput('info', "\n############################################")
    LogOutput('info', "3.3 Running" + tc_desc)
    LogOutput('info', "############################################\n")

    shell_cmd = "cp /etc/openswitch/supportability/ops_featuremapping.yaml\
     /etc/openswitch/supportability/ops_featuremapping.yaml2 "
    returnDevInt = dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    shell_cmd = 'printf  ---   -     feature_name: \"lldp\"' + \
        ' feature_desc: \"Link Layer Discovery Protocol\" ' + \
        ' daemon:       - \"ops-lldpd\" xxxxxxxxxxxxxxxxxxxxxxxxx ' + \
        ' aksjsjjdkdkdjddj kdkdkdkdkdkdkdk ---   ' + \
        ' -     feature_name: \"lldp\" ' + \
        '  feature_desc: \"Link Layer Discovery Protocol\" ' + \
        '    daemon:       - \"ops-lldpd\" xxxxxxxxxxxxxxxxxxxxxxxxx ' + \
        ' aksjsjjdkdkdjddj kdkdkdkdkdkdkdk ' + \
        ' ---   -     feature_name: \"lldp\"     feature_desc: ' + \
        ' \"Link Layer Discovery Protocol\"     daemon:       - \"ops-lldpd\"'\
        + '  xxxxxxxxxxxxxxxxxxxxxxxxx aksjsjjdkdkdjddj kdkdkdkdkdkdkdk' + \
        ' >>  /etc/openswitch/supportability/ops_featuremapping.yaml'

    dut01Obj.DeviceInteract(command=shell_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=True)
    returnDevInt = dut01Obj.DeviceInteract(command=vtysh_cmd)
    LogOutput('info', str(returnDevInt['buffer']))

    dut01Obj.VtyshShell(enter=False)

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    shell_cmd = "mv /etc/openswitch/supportability/ops_featuremapping.yaml2\
    /etc/openswitch/supportability/ops_featuremapping.yaml "
    dut01Obj.DeviceInteract(command=shell_cmd)

    if finalReturnCode != 0:
        LogOutput('error',
                  "Failed to run " + tc_desc +
                  " on device " + str(dut01Obj.device))
        return False
    else:
        if (str_check not in returnDevInt['buffer']):
            LogOutput(
                'error', tc_desc + "Test Case Failure,refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return False
        else:
            LogOutput('info',
                      tc_desc + "ran successfully on device " +
                      str(dut01Obj.device))
            return True


class Test_diag_dump:

    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_diag_dump.testObj = testEnviron(
            topoDict=topoDict)
        Test_diag_dump.topoObj = \
            Test_diag_dump.testObj.topoObjGet()
        # Global variables
        global dut01Obj
        dut01Obj = cls.topoObj.deviceObjGet(device="dut01")

    # positive test case
    def test_diag_dump_list(self):
        assert(checkDiagDumpList(dut01Obj))

    def test_diag_dump_feature(self):
        assert(checkDiagDumpFeature(dut01Obj, 'lldp'))

    def test_diag_dump_feature_file(self):
        assert(checkDiagDumpFeatureFile(dut01Obj, 'lldp', 'diag.txt'))

    def test_diag_dump_feature_file_path(self):
        assert(checkDiagDumpFeatureFilePath(dut01Obj, 'lldp', 'diag.txt'))

    def test_diag_dump_feature_file_size(self):
        assert(checkDiagDumpFeatureFileSize(dut01Obj, 'lldp', 'diag.txt'))

    # negative test case
    # When ops-bgpd daemon implement diag feature this TC will fail and they
    # can't commit their changes. In that case we have to identify some other
    # daemon which doesn't support diag feature
    def test_diag_dump_unsupported_daemon(self):
        assert(checkUnsupportedDaemon(dut01Obj, 'ops-bgpd'))

    def test_diag_dump_unknown_daemon(self):
        assert(checkUnknownDaemon(dut01Obj))

    def test_diag_dump_unknown_garb_daemon(self):
        assert(checkUnknownGarbDaemon(dut01Obj))

    def test_diag_dump_noconfig_file(self):
        assert(checkNoConfigfile(dut01Obj))

    def test_diag_dump_empty_config_file(self):
        assert(checkEmptyFile(dut01Obj))

    def test_diag_dump_corrupted_yaml_file(self):
        assert(checkCorruptedYamlFile(dut01Obj))

   # Teardown Class
    def teardown_class(cls):
        # Terminate all nodes
        Test_diag_dump.topoObj.terminate_nodes()
