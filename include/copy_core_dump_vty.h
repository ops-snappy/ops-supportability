/* COPY_CORE_DUMP CLI commands.
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
 * File: copy_core_dump_vty.h
 *
 * Purpose: To copy core dump to remote location.
 */

#ifndef _COPY_CORE_DUMP_VTY_H
#define _COPY_CORE_DUMP_VTY_H

void cli_pre_init(void);
void cli_post_init(void);

#define CORE_DUMP_STR \
   "Copy daemon or kernel coredump\n"
#define DAEMON_NAME_STR \
    "Specify daemon-name\n"
#define TFTP_CLIENT_STR \
    "Copy coredump to tftp server\n"
#define SFTP_CLIENT_STR \
    "Copy coredump to sftp server\n"
#define HOST_IPv4 \
    "Specify server IP\n"
#define HOST_NAME \
    "Specify server name\n"
#define FILENAME_STR \
   "Specify destination file name\n"
#define SFTP_USER_STR \
    "Specify user name of sshd server\n"
#define INSTANCE_ID_KEY_STR \
    "Coredump instance ID\n"
#define INSTANCE_ID_VALUE_STR \
    "Specify coredump instance ID\n"
#define KERNEL_STR \
    "Copy kernel coredump\n"

#define MAX_FILE_STR_LEN        512
#define MAX_LOG_STR_LEN         512
#define MAX_COMMAND_STR_LEN     1024

#define TFTP_NOI_SCRIPT         "/usr/bin/tftp_noi.sh"
#define SFTP_NOI_SCRIPT         "/usr/bin/sftp_noi.sh"
#define TFTP_STR                "tftp"
#define SFTP_STR                "sftp"

#define USER_NAME_REGEX         "^([A-Za-z0-9_-]){1,50}$"
#define HOST_NAME_REGEX         "^([A-Za-z0-9_.:-]){1,256}$"
#define FILE_NAME_REGEX         "^([A-Za-z0-9_.-]){1,50}$"
#define DAEMON_NAME_REGEX       "^([A-Za-z0-9_.-]){1,50}$"
/* corefile instance id is pid . It's range is 1 - 65536 ( 2^16) */
#define COREFILE_INSTANCE_REGEX "^([0-9]){1,5}$"

#define DAEMON_CORE_PATTERN     "*\\.xz"
#define KERNEL_CORE_PATTERN     "vmcore.[0-9][0-9][0-9][0-9][0-9][0-9][0-9]\
[0-9].[0-9][0-9][0-9][0-9][0-9][0-9].tar.gz"

#endif /* _COPY_CORE_DUMP_VTY_H */
