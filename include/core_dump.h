/* CORE_DUMP CLI commands.
 *
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
 * File: core_dump.h
 *
 * Purpose: To Run Copy, Show Core Dump command from CLI.
 */

#ifndef _CORE_DUMP_VTY_H
#define _CORE_DUMP_VTY_H

#include "supportability_utils.h"



#define GB_PATTERN    "%s/core*.xz"
#define CORE_FILE_PATTERN "core.*\\.xz"
#define KERN_GB_PATTERN \
   "vmcore.[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].[0-9][0-9][0-9]" \
"[0-9][0-9][0-9].tar.gz"

/* systemd uses hardcoded path to store core files
e.g.     "/var/lib/systemd/coredump\
/core.vtysh.0.68b62225067c4523af7d4d38e723da39.682.1459547856000000.xz" */

#define DAEMON_CORE_PATH        "\\/var\\/diagnostics\\/coredump"
#define KERNEL_CORE_PATH        "\\/var\\/diagnostics\\/coredump"


#define KERN_CORE_FILE_PATTERN "([0-9]{8})\\.([0-9]{6})\\.tar\\.gz"

#define DATE_STR_SIZE         11
#define TIME_STR_SIZE         9
#define SIGNAL_STR_SIZE       3
#define INDEX_STR_SIZE        4
#define SRC_DATE_STR_LEN      8
#define SRC_TIME_STR_LEN      6
#define SRC_SIGNAL_STR_LEN    2
#define DEAMON_NAME_SIZE      256
#define REGEX_COMP_ERR        1000
#define CORE_LOC_CONFIG       300
#define CORE_FILE_NAME        600
#define MIN_SIZE              6
#define INSTANCE_ID_SIZE      8
#define SIZE_DATE_AND_TIME    20
/*  we have 5 information to extract from file name.
 *  They are  daemonname,time,date, index and signal.
 *  Together with the full match
 *  the number of groups become 6*/
#define TOTAL_INFO             6

enum
{
   TYPE_KERNEL,
   TYPE_DAEMON
};

struct core_dump_data {
   char daemon_name[DEAMON_NAME_SIZE+1];
   char crash_date[DATE_STR_SIZE+1];
   char crash_time[TIME_STR_SIZE+1];
   char crash_signal[SIGNAL_STR_SIZE+1];
   char crash_instance_id[INSTANCE_ID_SIZE+1];
};

int
extract_info (
      regex_t * regexst, const char * filename,
      struct core_dump_data* cd,int type);

int
get_file_list(int type, glob_t* globbuf, const char* globpattern ,
        const char *daemon, const char* instance_id );

int
validate_cli_args(const char * arg , const char * regex);

#endif //_CORE_DUMP_VTY_H
