/* System COPY_CORE_DUMP CLI commands
*
* Copyright (C) 1997, 98 Kunihiro Ishiguro
* Copyright (C) 2016 Hewlett Packard Enterprise Development LP
*
* GNU Zebra is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2, or (at your option) any
* later version.
*
* GNU Zebra is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GNU Zebra; see the file COPYING.  If not, write to the Free
* Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
* 02111-1307, USA.
*
* File: copy_core_dump_vty.c
*
* Purpose: To copy core dump from the switch
*/

#include <glob.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <libgen.h>


#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "copy_core_dump_vty.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"
#include "core_dump.h"
#include "supportability_utils.h"

VLOG_DEFINE_THIS_MODULE (vtysh_copy_core_dump_cli);



/* Function       : cli_copy_core_dump
 * Resposibility  : Copy core dump to the destination using tftp or sftp
 * Parameters
 *                : daemon_name - daemon name string
 *                : instance_id - instance id of the core-dump
 *                : protocol  - "tftp" or "sftp"
 *                : address  -  hostname of ip address of tftp/sshd server
 *                : user_name - username of sshd server . This argument is valid
 *                              for sftp option
 *                : destination_file - (optional ) destination file name
 *
 * Returns        : 0 on success
 */

int
cli_copy_core_dump(const char* daemon_name,const char* instance_id,
      const char* protocol, const char* address,const char* user_name,
      const char* destination_file )
{
    glob_t globbuf;
    size_t i=0;
    char *gb_pattern=NULL;
    struct stat sb;

    /* We need minimum 4 arg for sftp command */
    int argc = 4;
    char *arguments[argc];

    int rc = 0;
    char command[MAX_COMMAND_STR_LEN] = {0};
    char file_name[MAX_FILE_STR_LEN] = {0};
    char log_str[MAX_LOG_STR_LEN] = {0};
    char * file_str_ptr = NULL;
    int type = -1;

    /* validate mandatory parameters
       optional parameters can be NULL
    */
    if (( daemon_name && protocol && address ) ==  0  ) {
        vty_out(vty,"Invalid parameter%s",VTY_NEWLINE);
        VLOG_ERR("Invalid parameter");
        return CMD_WARNING;
    }

    /* validate based on regular expression */
    rc = validate_cli_args(daemon_name,DAEMON_NAME_REGEX);
    if ( rc != 0) {
        vty_out(vty,"Failed to validate daemon name:%s%s",
                daemon_name,VTY_NEWLINE);
        VLOG_ERR("Failed to validate daemon name:%s,rc:%d",
                daemon_name,rc);
        return CMD_WARNING;
    }

    rc = validate_cli_args(address, HOST_NAME_REGEX);
    if ( rc != 0) {
        vty_out(vty,"Failed to validate hostname name:%s%s",
                address,VTY_NEWLINE);
        VLOG_ERR("Failed to validate hostname name:%s,rc:%d",
                address,rc);
        return CMD_WARNING;
    }


    if  ( instance_id != NULL ) {
        rc = validate_cli_args(instance_id,COREFILE_INSTANCE_REGEX);
        if ( rc != 0) {
            vty_out(vty,"Failed to validate instance ID:%s%s",
                    instance_id, VTY_NEWLINE);
            VLOG_ERR("Failed to validate instance ID:%s,rc:%d",
                    instance_id, rc);
            return CMD_WARNING;
        }
    }

    if ( destination_file != NULL )  {
        rc = validate_cli_args(destination_file,FILE_NAME_REGEX);
        if ( rc != 0) {
            vty_out(vty,"Failed to validate destination file name:%s%s",
                    destination_file, VTY_NEWLINE);
            VLOG_ERR("Failed to validate destination file name:%s,rc:%d",
                    destination_file,rc);
            return CMD_WARNING;
        }
    }


    /* Validate protocol and checking existance of binary */
    if ( 0 == strncmp_with_nullcheck( protocol , TFTP_STR , strlen(TFTP_STR))) {
        strncpy (command,TFTP_NOI_SCRIPT ,sizeof(command));
        STR_SAFE(command);
        if  (
                !(
                    (0 == stat(command , &sb)) &&
                    (S_ISREG(sb.st_mode)) &&
                    (sb.st_mode & S_IXGRP)
                 )
            )
        {
            vty_out(vty,"Utility not available for execution:%s%s",command,
                    VTY_NEWLINE);
            VLOG_ERR("Utility not available for execution:%s", command);
            return CMD_WARNING;
        }
    }
    else if (0 == strncmp_with_nullcheck ( protocol , SFTP_STR ,
                strlen( SFTP_STR )))  {

        if ( user_name == NULL ) {
            vty_out(vty,"Invalid parameter username%s",VTY_NEWLINE);
            VLOG_ERR("Invalid parameter username");
            return CMD_WARNING;
        }

        rc = validate_cli_args(user_name,USER_NAME_REGEX);
        if ( rc != 0) {
            vty_out(vty,"Failed to validate user name for sftp:%s%s",
                    user_name, VTY_NEWLINE);
            VLOG_ERR("Failed to validate user name for sftp:%s , rc:%d",
                    user_name,rc);
            return CMD_WARNING;
        }


        strncpy (command, SFTP_NOI_SCRIPT, sizeof(command));
        STR_SAFE(command);
        if  (
                !(
                    (0 == stat(command , &sb)) &&
                    (S_ISREG(sb.st_mode)) &&
                    (sb.st_mode & S_IXGRP)
                 )
            )
        {
            vty_out(vty,"Utility not available for execution:%s%s",command,
                    VTY_NEWLINE);
            VLOG_ERR("Utility not available for execution:%s", command);
            return CMD_WARNING;
        }
    }
    else {
        vty_out(vty,"Invalid parameter protocol :%s%s",protocol,VTY_NEWLINE);
        VLOG_ERR("Invalid parameter protocol :%s",protocol);
        return CMD_WARNING;
    }
    /* end of cli parameter validation */


    if ( 0 == strncmp_with_nullcheck((char*)daemon_name,"kernel",10))
    {
        type = TYPE_KERNEL;
        gb_pattern = KERNEL_CORE_PATTERN;
    }
    else
    {
        /* systemd uses hardcoded path for coredump
           We dont need to read any config file for this */
        type = TYPE_DAEMON;
        gb_pattern = DAEMON_CORE_PATTERN ;
    }

    rc = get_file_list(type, &globbuf, gb_pattern ,
            daemon_name, instance_id);
    if ( rc != 0 )
    {
        vty_out(vty,"Error to listing corefiles %s",VTY_NEWLINE);
        VLOG_ERR("Glob failed to listing core files:%d", rc );
        globfree(&globbuf);
        return CMD_WARNING;
    }

    if ( globbuf.gl_pathc <= 0 )
    {
        if ( instance_id ) {
            if ( type == TYPE_KERNEL )
                snprintf(log_str, sizeof(log_str),
                        "No coredump found for kernel");
            else
                snprintf(log_str, sizeof(log_str),
                        "No coredump found for daemon %s with instance %s",
                        daemon_name, instance_id );
        }
        else {
            if ( type == TYPE_KERNEL )
                snprintf(log_str, sizeof(log_str),
                        "No coredump found for kernel");
            else
                snprintf(log_str, sizeof(log_str),
                        "No coredump found for daemon %s", daemon_name);
        }
        STR_SAFE(log_str);

        vty_out(vty,"%s%s",log_str, VTY_NEWLINE);
        VLOG_DBG(log_str);
        globfree(&globbuf);
        return CMD_SUCCESS;
    }


    for (i = 0; i < globbuf.gl_pathc;i++)
    {

        if ( destination_file == NULL ||
                destination_file[0] == ' ' ||
                destination_file[0] == '\t' ) {

            file_str_ptr  = basename(globbuf.gl_pathv[i]);
            if ( file_str_ptr == NULL ) {
                vty_out(vty,"Failed to get filename%s",VTY_NEWLINE);
                VLOG_ERR("Failed to get filename");
                globfree(&globbuf);
                return CMD_WARNING;
            }
            strncpy(file_name,file_str_ptr,sizeof(file_name));
        }
        else {
            strncpy(file_name,destination_file,sizeof(file_name));
        }
        STR_SAFE(file_name);

        if ( 0 == strncmp_with_nullcheck(protocol, TFTP_STR, strlen(TFTP_STR)))
        {
            argc = 3;
            arguments[0] = (char*) address;
            arguments[1] = (char*) globbuf.gl_pathv[i];
            arguments[2] = (char*) file_name;
        }

        if ( 0 == strncmp_with_nullcheck(protocol, SFTP_STR, strlen(SFTP_STR)))
        {
            argc = 4;
            arguments[0] = (char*) user_name ;
            arguments[1] = (char*) address ;
            arguments[2] = (char*) globbuf.gl_pathv[i];
            arguments[3] = (char*) file_name ;
        }
        rc = execute_command(command , argc, (const char **)arguments);
        if ( rc != 0) {
            vty_out(vty,"%s command is failed to execute%s",
                    command,VTY_NEWLINE);
            VLOG_ERR("%s command is failed to execute",command);
            globfree(&globbuf);
            return CMD_WARNING;
        }
        /* Kernel Core file configuration is configured to generate only one
         * core file. We don't expect more than one vmcore file  */
        if (type == TYPE_KERNEL)
            break;
    }

    globfree(&globbuf);
    return CMD_SUCCESS;
}


/*
* Action routines for copy core dump CLI using tftp
* This cli don't support destination file name.
* Reason: one damemon can have multiple corefiles and this cli copies all
* corefile of a daemon , name remaining same as source filename.
*/
DEFUN_NOLOCK (cli_platform_copy_core_dump_tftp,
        cli_platform_copy_core_dump_tftp_cmd,
        "copy core-dump DAEMON_NAME tftp (A.B.C.D | WORD)",
        COPY_STR
        CORE_DUMP_STR
        DAEMON_NAME_STR
        TFTP_CLIENT_STR
        HOST_IPv4
        HOST_NAME)
{
    return cli_copy_core_dump(argv[0], NULL, TFTP_STR, argv[1], NULL, NULL );
}


/*
* Action routines for copy core dump CLI using sftp
* This cli don't support destination file name.
* Reason: one damemon can have multiple corefiles and this cli copies all
* corefile of a daemon , name remaining same as source filename.
*/
DEFUN_NOLOCK (cli_platform_copy_core_dump_sftp,
        cli_platform_copy_core_dump_sftp_cmd,
        "copy core-dump DAEMON_NAME sftp USERNAME (A.B.C.D | WORD)",
        COPY_STR
        CORE_DUMP_STR
        DAEMON_NAME_STR
        SFTP_CLIENT_STR
        SFTP_USER_STR
        HOST_IPv4
        HOST_NAME)
{
    return cli_copy_core_dump(argv[0], NULL, SFTP_STR, argv[2], argv[1], NULL );
}


/*
* Action routines for copy core dump CLI using tftp
* This cli don't support destination file name.
* Reason: one damemon can have multiple corefiles and this cli copies all
* corefile of a daemon , name remaining same as source filename.
*/
DEFUN_NOLOCK (cli_platform_copy_core_dump_kernel_tftp,
        cli_platform_copy_core_dump_kernel_tftp_cmd,
        "copy core-dump kernel tftp (A.B.C.D | WORD) [FILENAME]",
        COPY_STR
        CORE_DUMP_STR
        KERNEL_STR
        TFTP_CLIENT_STR
        HOST_IPv4
        HOST_NAME
        FILENAME_STR)
{
    return cli_copy_core_dump("kernel", NULL, TFTP_STR, argv[0],NULL,
            /* additional check for optional parameter FILE_NAME */
            ( ( argc >= 2 ) ? argv[1] : NULL ));
}


/*
* Action routines for copy core dump CLI using sftp
* This cli don't support destination file name.
* Reason: one damemon can have multiple corefiles and this cli copies all
* corefile of a daemon , name remaining same as source filename.
*/
DEFUN_NOLOCK (cli_platform_copy_core_dump_kernel_sftp,
        cli_platform_copy_core_dump_kernel_sftp_cmd,
        "copy core-dump kernel sftp USERNAME (A.B.C.D | WORD) [FILENAME]",
        COPY_STR
        CORE_DUMP_STR
        KERNEL_STR
        SFTP_CLIENT_STR
        SFTP_USER_STR
        HOST_IPv4
        HOST_NAME
        FILENAME_STR)
{
    return cli_copy_core_dump("kernel", NULL, SFTP_STR,argv[1],argv[0],
            /* additional check for optional parameter FILE_NAME */
            ( ( argc >= 3 ) ? argv[2] : NULL ));

}

/*
* Action routines for copy core dump CLI using tftp
*/
DEFUN_NOLOCK (cli_platform_copy_core_dump_tftp_inst,
        cli_platform_copy_core_dump_tftp_cmd_inst,
        "copy core-dump DAEMON_NAME instance-id INSTANCE_ID"
        " tftp (A.B.C.D | WORD) [FILE_NAME]",
        COPY_STR
        CORE_DUMP_STR
        DAEMON_NAME_STR
        INSTANCE_ID_KEY_STR
        INSTANCE_ID_VALUE_STR
        TFTP_CLIENT_STR
        HOST_IPv4
        HOST_NAME
        FILENAME_STR)
{
    return cli_copy_core_dump(argv[0], argv[1], TFTP_STR, argv[2], NULL,
            /* additional check for optional parameter FILE_NAME */
            ( ( argc >= 4 ) ? argv[3] : NULL ));
}


/*
 * Action routines for copy core dump CLI using sftp
 */
DEFUN_NOLOCK (cli_platform_copy_core_dump_sftp_inst,
        cli_platform_copy_core_dump_sftp_cmd_inst,
        "copy core-dump DAEMON_NAME instance-id INSTANCE_ID sftp"
        " USERNAME (A.B.C.D | WORD) [FILE_NAME]",
        COPY_STR
        CORE_DUMP_STR
        DAEMON_NAME_STR
        INSTANCE_ID_KEY_STR
        INSTANCE_ID_VALUE_STR
        SFTP_CLIENT_STR
        SFTP_USER_STR
        HOST_IPv4
        HOST_NAME
        FILENAME_STR)
{
    return cli_copy_core_dump(argv[0], argv[1], SFTP_STR, argv[3], argv[2],
             /* additional check for optional parameter FILE_NAME */
            ( ( argc >= 5 ) ? argv[4] : NULL ));
}
