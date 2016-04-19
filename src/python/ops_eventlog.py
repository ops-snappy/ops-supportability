#!/usr/bin/env python
# (c) Copyright [2016] Hewlett Packard Enterprise Development LP
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

from systemd import journal
import yaml
import ovs.vlog

content = []
category = []

FAIL = -1
NOT_FOUND = 0
EV_CATEGORY = "event_category"
EV_DEFINITION = "event_definitions"
EV_NAME = "event_name"
EV_ID = "event_ID"
EV_SEVERITY = "severity"
EV_DESCRIPTION = "description"
EV_DESCRIPTION_YAML = "event_description_template"


# Logging.
vlog = ovs.vlog.Vlog("ops-eventlog")

# Initialization API for event Log category


def event_log_init(cat):
    global content
    global category
    found = 0
# Search whether category is already initialised
    for i in range(len(category)):
        if cat in category[i]:
# Already initialised, so return.
            return FAIL
    try:
        with open('/etc/openswitch/supportability/ops_events.yaml', 'r') as f:
            doc = yaml.load(f)
            f.close()
            for txt in range(len(doc[EV_DEFINITION])):
                yaml_cat = doc[EV_DEFINITION][txt][EV_CATEGORY]
                if yaml_cat == cat:
                    mydic = {
                        EV_CATEGORY: yaml_cat,
                        EV_NAME: doc[EV_DEFINITION][txt][EV_NAME],
                        EV_ID: doc[EV_DEFINITION][txt][EV_ID],
                        EV_SEVERITY: doc[EV_DEFINITION][txt][EV_SEVERITY],
                        EV_DESCRIPTION: doc[EV_DEFINITION][
                            txt][EV_DESCRIPTION_YAML],
                    }
# Now add it to global event list
                    content.append(mydic)
                    found = 1

            if found is NOT_FOUND:
# This means supplied category name is not there in YAML, so return.
                vlog.err("Event Category not Found")
                return FAIL
            else:
# Add category to global category list
                category.append(cat)

    except:
        vlog.err("Event Log Initialization Failed")
        return FAIL

# Utility API used to replace key with value provided.


def replace_str(keys, desc):
    for j in range(len(keys)):
        key = keys[j][0]
        key = "{" + key + "}"
        value = keys[j][1]
        desc = desc.replace(str(key), str(value))
    return desc

# API to log events from a python daemon


def log_event(name, *arg):
    found = 0
    for i in range(len(content)):
        if name == content[i][EV_NAME]:
# Found the event in list!
            ev_id = str(content[i][EV_ID])
            severity = content[i][EV_SEVERITY]
            desc = content[i][EV_DESCRIPTION]
            categ = content[i][EV_CATEGORY]
            if len(arg):
                desc = replace_str(arg, desc)
            found = 1
            break;
    if found is NOT_FOUND:
# This means supplied event name is not there in YAML, so return.
        vlog.err("Event not Found")
        return FAIL
    mesg = 'ops-evt|' + ev_id + '|' + severity + '|' + desc
    journal.send(
        mesg, MESSAGE_ID='50c0fa81c2a545ec982a54293f1b1945', PRIORITY=severity,
        OPS_EVENT_ID=ev_id, OPS_EVENT_CATEGORY=categ)
