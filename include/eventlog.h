/*
 Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
 All Rights Reserved.

    Licensed under the Apache License, Version 2.0 (the "License"); you may
    not use this file except in compliance with the License. You may obtain
    a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
    License for the specific language governing permissions and limitations
    under the License.
*/

/************************************************************************//**
 * @defgroup ops_supportability
 * This library provides event logging functions used by various OpenSwitch
 * processes.
 * @{
 *
 * @defgroup ops_supportability_public Public Interface
 * Public API for ops_supportability library.
 *
 * - EVENGLOG: A set of functions used for logging events
 *
 * @{
 *
 * @file
 * Header for event logging infra.
 ***************************************************************************/

#ifndef __EVENTLOG_H_
#define __EVENTLOG_H_

#define TRUE 1
#define FALSE 0
#define MESSAGE_OPS_EVT "50c0fa81c2a545ec982a54293f1b1945"
#define MAX_CATEGORIES_PER_DAEMON 99
#define KEY_VALUE_SIZE 128
#define MAX_LOG_STR 480
#define MAX_EVENT_NAME_SIZE 64
#define MAX_SEV_NAME_SIZE 10
#define MAX_EVENT_TABLE_SIZE 500
#define EVENT_NAME_DELIMITER_STR "EV_TBD_TBD"
#define EVENT_YAML_FILE "/etc/openswitch/supportability/ops_events.yaml"
#define MAX_SEV_LEVELS 8
#define EV_KV(...) key_value_string(__VA_ARGS__)


typedef struct {
    char *category;
    int event_id;
    char event_name[MAX_EVENT_NAME_SIZE];
    char severity[MAX_SEV_NAME_SIZE];
    int num_of_keys;
    char event_description[MAX_LOG_STR];
    } event;

extern int event_log_init(char *category);
extern int log_event(char *ev_name,...);
extern char *key_value_string(char *s1, ...);
#endif /* __EVENTLOG_H_ */
