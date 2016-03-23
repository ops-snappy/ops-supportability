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

import re
from opstestfw import testEnviron, LogOutput


topoDict = {"topoExecution": 1000,
            "topoTarget": "dut01",
            "topoDevices": "dut01",
            "topoFilters": "dut01:system-category:switch"}


# Negative test case for show events severity filter
def evtlogfilterSeverity_cli(dut01):
    LogOutput('info', "\n############################################")
    LogOutput('info', " Running Event Log Test Script")
    LogOutput('info', "############################################\n")
    overallBuffer = []
    finalReturnCode = 0

    #Get in to vtysh shell
    retStruct = dut01.VtyshShell(enter=True)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode != 0:
        LogOutput('error',"Failed to enter vtysh prompt")
        for curLine in overallBuffer:
                LogOutput('info',str(curLine))
        return False
    # enable lldp
    dut01.DeviceInteract(command="configure terminal")
    dut01.DeviceInteract(command="lldp enable")
    #disable lldp
    dut01.DeviceInteract(command="no lldp enable")
    dut01.DeviceInteract(command="lldp timer 200")

    dut01.DeviceInteract(command="end")

    print "-"*10
    print "=====----"
    print "-"*10

    returnDevInt = dut01.DeviceInteract(command="show events severity emer")
    retStruct = dut01.VtyshShell(enter=False)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode !=0:
        LogOuput('error',"Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info',str(curLine))
        return False
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                "Failed to run show events " +
                "on device" + str(dut01))
        return False
    else:
        if "No event match the filter provided" in returnDevInt.get('buffer'):
            LogOutput('info',
                    "Test Case passed, please refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return True
        else:
            return False

# Negative test case for show events category filter
def evtlogfilterCategory_cli(dut01):
    LogOutput('info', "\n############################################")
    LogOutput('info', " Running Event Log Test Script")
    LogOutput('info', "############################################\n")
    overallBuffer = []
    finalReturnCode = 0

    #Get in to vtysh shell
    retStruct = dut01.VtyshShell(enter=True)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode != 0:
        LogOutput('error',"Failed to enter vtysh prompt")
        for curLine in overallBuffer:
                LogOutput('info',str(curLine))
        return False
    # enable lldp
    dut01.DeviceInteract(command="configure terminal")
    dut01.DeviceInteract(command="lldp enable")
    #disable lldp
    dut01.DeviceInteract(command="no lldp enable")
    dut01.DeviceInteract(command="lldp timer 200")

    dut01.DeviceInteract(command="end")

    print "-"*10
    print "=====----"
    print "-"*10

    returnDevInt = dut01.DeviceInteract(command="show events category abc")
    retStruct = dut01.VtyshShell(enter=False)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode !=0:
        LogOuput('error',"Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info',str(curLine))
        return False
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                "Failed to run show events " +
                "on device" + str(dut01))
        return False
    else:
        if "No event match the filter provided" in returnDevInt.get('buffer'):
            LogOutput('info',
                    "Test Case passed, please refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return True
        else:
            return False

# Negative test case for show events event id filter
def evtlogfilterEventID_cli_fail(dut01):
    LogOutput('info', "\n############################################")
    LogOutput('info', " Running Event Log Test Script 1.2")
    LogOutput('info', "############################################\n")
    overallBuffer = []
    finalReturnCode = 0

    #Get in to vtysh shell
    retStruct = dut01.VtyshShell(enter=True)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode != 0:
        LogOutput('error',"Failed to enter vtysh prompt")
        for curLine in overallBuffer:
                LogOutput('info',str(curLine))
        return False
    print "-"*10
    print "=====----"
    print "-"*10
    returnDevInt = dut01.DeviceInteract(command="show events event-id 999998")
    retStruct = dut01.VtyshShell(enter=False)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode !=0:
        LogOuput('error',"Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info',str(curLine))
        return False
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                "Failed to run show events " +
                "on device" + str(dut01))
        return False
    else:
        if "No event match the filter provided" in returnDevInt.get('buffer'):
            LogOutput('info',
                    "Test Case passed, please refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return True
        else:
            return False

# Positive test case for show events event id filter
def evtlogfilterEventID_cli_pass(dut01):
    LogOutput('info', "\n############################################")
    LogOutput('info', " Running Event Log Test Script 1.1")
    LogOutput('info', "############################################\n")
    overallBuffer = []
    finalReturnCode = 0

    #Get in to vtysh shell
    retStruct = dut01.VtyshShell(enter=True)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode != 0:
        LogOutput('error',"Failed to enter vtysh prompt")
        for curLine in overallBuffer:
                LogOutput('info',str(curLine))
        return False

    # enable lldp
    dut01.DeviceInteract(command="configure terminal")
    dut01.DeviceInteract(command="lldp enable")
    #disable lldp
    dut01.DeviceInteract(command="no lldp enable")
    dut01.DeviceInteract(command="lldp timer 100")

    dut01.DeviceInteract(command="end")

    print "-"*10
    print "=====----"
    print "-"*10

    returnDevInt = dut01.DeviceInteract(command="show events event-id 1003")
    retStruct = dut01.VtyshShell(enter=False)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode !=0:
        LogOuput('error',"Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info',str(curLine))
        return False
    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                "Failed to run show events " +
                "on device" + str(dut01))
        return False
    else:
        if "Configured LLDP tx-timer with" and "LLDP Disabled" not in returnDevInt.get('buffer'):
            LogOutput('info',
                    "Test Case passed, please refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return True
        else:
            return False

def evtlogfeature_cli(dut01):
    LogOutput('info', "\n############################################")
    LogOutput('info', " Running Event Log Test Script")
    LogOutput('info', "############################################\n")
    overallBuffer = []
    finalReturnCode = 0

    #Get in to vtysh shell
    retStruct = dut01.VtyshShell(enter=True)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode != 0:
        LogOutput('error',"Failed to enter vtysh prompt")
        for curLine in overallBuffer:
                LogOutput('info',str(curLine))
        return False

    # enable lldp
    dut01.DeviceInteract(command="configure terminal")
    dut01.DeviceInteract(command="lldp enable")
    #disable lldp
    dut01.DeviceInteract(command="no lldp enable")
    dut01.DeviceInteract(command="lldp timer 100")

    dut01.DeviceInteract(command="end")

    print "-"*10
    print "=====----"
    print "-"*10

    returnDevInt = dut01.DeviceInteract(command="show events")
    retStruct = dut01.VtyshShell(enter=False)
    overallBuffer.append(retStruct.buffer())
    returnCode = retStruct.returnCode()
    if returnCode !=0:
        LogOuput('error',"Failed to exit vtysh prompt")
        for curLine in overallBuffer:
            LogOutput('info',str(curLine))
        return False

    finalReturnCode = returnDevInt['returnCode']
    overallBuffer.append(returnDevInt['buffer'])

    if finalReturnCode != 0:
        LogOutput('error',
                "Failed to run show events " +
                "on device" + str(dut01))
        return False
    else:
        if "LLDP Enabled" or "LLDP Disabled" in returnDevInt.get('buffer'):
            LogOutput('info',
                    "Test Case passed, please refer output below")
            for outputs in overallBuffer:
                LogOutput('info', str(outputs))
            return True
        else:
            return False


class Test_ft_evtlog_feature:
    def setup_class(cls):
        # Create Topology object and connect to devices
        Test_ft_evtlog_feature.testObj = \
                testEnviron(topoDict=topoDict)
        Test_ft_evtlog_feature.topoObj = \
            Test_ft_evtlog_feature.testObj.topoObjGet()

    def teardown_class(cls):
        # Terminate all nodes
        Test_ft_evtlog_feature.topoObj.terminate_nodes()

    # Test basic show events command
    def test_show_events(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = evtlogfeature_cli(dut01Obj)
        if retValue == 1:
            LogOutput('info', "Event log  CLI -passed")
        else:
            LogOutput('info', "Event log CLI -failed")

    # Test show events event ID FIlter (Positive & negative case)
    def test_show_events_eventID(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        # Positive test case
        retValue = evtlogfilterEventID_cli_pass(dut01Obj)
        if retValue == 1:
            LogOutput('info', "Event log event ID filter positive case -passed")
        else:
            LogOutput('info', "Event log event ID filter positive case -failed")
        # Negative
        retValue = evtlogfilterEventID_cli_fail(dut01Obj)
        if retValue == 1:
            LogOutput('info', "Event log  event ID filter negative case -passed")
        else:
            LogOutput('info', "Event log event ID filter negative case -failed")

    # Test show events event category Filter(negative case)
    def test_show_events_category(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = evtlogfilterCategory_cli(dut01Obj)
        if retValue == 1:
            LogOutput('info', "Event log category filter  CLI -passed")
        else:
            LogOutput('info', "Event log category filter CLI -failed")

    # Test show events severity negative test case
    def test_show_events_severity(self):
        dut01Obj = self.topoObj.deviceObjGet(device="dut01")
        retValue = evtlogfilterSeverity_cli(dut01Obj)
        if retValue == 1:
            LogOutput('info', "Event log category filter  CLI -passed")
        else:
            LogOutput('info', "Event log category filter CLI -failed")
