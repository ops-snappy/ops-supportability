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
 * Purpose: To Run Show Core Dump command from CLI.
 */

#ifndef _CORE_DUMP_VTY_H
#define _CORE_DUMP_VTY_H

#include "supportability_utils.h"


#define GB_PATTERN \
   "%s/{,*/}*.[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].[0-9][0-9][0-9]" \
"[0-9][0-9][0-9].core.tar.gz"

#define CORE_FILE_PATTERN "([a-zA-Z0-9_\\-]+)\\.([0-9]{1,3})\\.([0-9]" \
   "{8})\\.([0-9]{6})\\.core\\.tar\\.gz"

#define KERN_GB_PATTERN \
   "%s/vmcore.[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].[0-9][0-9][0-9]" \
"[0-9][0-9][0-9].tar.gz"

#define KERN_CORE_FILE_PATTERN "([0-9]{8})\\.([0-9]{6})\\.tar\\.gz"

#define CORE_DUMP_CONFIG      "/etc/ops_corefile.conf"
#define KERNEL_DUMP_CONFIG    "/etc/kdump.conf"
#define DATE_STR_SIZE         11
#define TIME_STR_SIZE         9
#define INDEX_STR_SIZE        4
#define SRC_DATE_STR_LEN      8
#define SRC_TIME_STR_LEN      6
#define FILENAME_SIZE         256
#define REGEX_COMP_ERR        1000
#define CORE_LOC_CONFIG       300
#define CORE_FILE_NAME        600

/* we have 4 information to extract from file name.
 *  * They are  daemonname,time,date and index. Together with the full match
 *   * the number of groups become 5*/
#define TOTAL_INFO  5

enum
{
   TYPE_KERNEL,
   TYPE_DAEMON
};

struct core_dump_data {
   char daemon_name[FILENAME_SIZE];
   char crash_index[INDEX_STR_SIZE];
   char crash_date[DATE_STR_SIZE];
   char crash_time[TIME_STR_SIZE];
};

int
extract_info (
      regex_t * regexst, const char * filename,struct core_dump_data* cd,int type);

int
get_file_list(const char* filepath,int type,
      glob_t* globbuf, const char* globpattern );



#endif //_CORE_DUMP_VTY_H
