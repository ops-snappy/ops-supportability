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

/*************************************************************************//**
 * @ingroup ops_supportability
 * This module contains the DEFINES and functions that comprise the event log
 * part of supportability library.
 *
 * @file
 * Source file for eventlog part of supportability library.
 *
 ****************************************************************************/

#include <stdio.h>
#include "eventlog.h"

/* Sample API : Will be replaced with EventLog Functions */
int
event_log (const char *log_message)
{
   printf("ELog : %s\n",log_message);
   return 0;
}
