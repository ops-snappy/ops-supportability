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
 * This library provides diagnostic dump registration function used by
 * used by various OpenSwitch processes.
 *
 * @defgroup ops_supportability_public Public Interface
 * Public API for ops_supportability library.
 *
 *
 * @file
 * Header for diagnostic dump
 ***************************************************************************/

#ifndef __DIAG_DUMP_H_
#define __DIAG_DUMP_H_

#include "dirs.h"
#include "util.h"
#include "daemon.h"
#include "unixctl.h"

#define DIAG_DUMP_BASIC_CMD     "dumpdiagbasic"
#define DIAG_DUMP_ADVANCED_CMD  "dumpdiagadvanced"

#define DIAG_BASIC              "basic"
#define DIAG_ADVANCED           "advanced"

/*
 * Macro            : INIT_DIAG_DUMP_BASIC
 * Responsibility   : It will register handler function for basic diag-dump.
 *                    handler function will dynamically allocate memory and
 *                    populate data. diag_handler_cb function will send
 *                    unixctl-reply to vtysh and free  dynamically allocated
 *                    memory.
 *
 * Parameters       :  CB_BASIC - callback handler
 *
 */

#define INIT_DIAG_DUMP_BASIC(CB_BASIC) \
void diag_handler_cb (struct unixctl_conn *conn, int argc ,\
                   const char *argv[], void *aux OVS_UNUSED)\
{\
    char *buf = NULL;\
    char err_desc[200];\
/*    argv[0] is  DIAG_DUMP_BASIC_CMD , \
      argv[1] is  DIAG_BASIC , \
      argv[2] is  feature name */\
    CB_BASIC(argv[2],&buf);\
    if (buf){\
        unixctl_command_reply(conn, buf);\
        free(buf);\
    } else {\
        snprintf(err_desc,sizeof(err_desc),\
                "%s feature failed to provide basic diagnostic data",argv[2]);\
        unixctl_command_reply_error(conn, err_desc);\
    }\
    return;\
}\
unixctl_command_register(DIAG_DUMP_BASIC_CMD,"",2,2,diag_handler_cb,NULL);
#endif /* __DIAG_DUMP_H_ */
