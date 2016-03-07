
/* Diagnostic dump CLI commands file
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: diag_dump_vty.h
 *
 * Purpose: header file for diag_dump_vty.c
 */

#ifndef __DIAG_DUMP_VTY_H
#define __DIAG_DUMP_VTY_H

#include "vtysh/command.h"
#include "dirs.h"
#include "util.h"
#include "daemon.h"
#include "unixctl.h"
#include "diag_dump.h"
#include "feature_mapping.h"

#define DIAG_DUMP_DIR         "/tmp/ops-diag"
#define FILE_PATH_LEN_MAX          256
#define MAX_TIME_STR_LEN           256
#define MAX_CLI_STR_LEN            256
#define USER_FILE_LEN_MAX          50
#define DIAG_CMD_LEN_MAX           50

#define MAX_PID      65536
#define MIN_PID      1
#define MAX_PID_LEN  5
#define MIN_PID_LEN  1

#define DIAG_DUMP_STR              "Show diagnostic information\n"
#define DIAG_DUMP_LIST_STR         "Show supported features with description\n"

#define DIAG_DUMP_FEATURE          "Feature name  \n"
#define DIAG_DUMP_FEATURE_BASIC    "Basic information \n"
#define DIAG_DUMP_FEATURE_FILE     "Relative path of file\n"


#define FREE(X)\
        if(X) { free (X); X = NULL; }

#define CLOSE(X)\
        if(X > 0)  { close(X); X = -1; }

#define STR_NULL_CHK(X)\
        (X)?(X):"NULL"

#define VALID_FD_CHECK(X)\
        ( ( (X) < 0) ? 0 : 1 )

#define  STR_SAFE(X)\
        if (sizeof(X) >=  1 )   X[ sizeof(X) - 1 ] =  '\0' ;

#define CLI_STR_EQUAL \
                    "==========================================================\
==============="
#define CLI_STR_HYPHEN \
                    "----------------------------------------------------------\
---------------"

int strcmp_with_nullcheck( const char * str1, const char * str2 );

#endif /* __DIAG_DUMP_VTY_H */
