/*
 Copyright (C) 2015 Hewlett-Packard Development Company, L.P.
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



/************************************************************************//**
 * Logs the Event in the Logging Infra
 *
 * @param[in] log_message   : Event Message to be Logged
 *
 * @return int status of log action
 ***************************************************************************/
extern int event_log (const char *log_message);

#endif /* __EVENTLOG_H_ */
