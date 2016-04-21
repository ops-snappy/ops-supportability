/* System CORE DUMP CLI Library
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
* File: core_dump.c
*
* Purpose: Library Routines for Core Dump CLI
*/

#include <glob.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "core_dump.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"

VLOG_DEFINE_THIS_MODULE (core_dump_lib);

/*
Function : convert_to_date_and_time
Responsibility  : Convert Unix time to Date and Time Formate {Date: 2016-03-28 <%Y-%m-%d> & Time : 09:59:59 <%H:%M:%S>}
*/
void
convert_to_date_and_time(char *buf_date, char * buf_time, int buf_date_size, int buf_time_size, const char *str)
{
    int strl = 0;
    char timestring[TIME_STR_SIZE] = {0,};
    char datestring[DATE_STR_SIZE] = {0,};
    time_t t_t = 0;
    struct tm *tmp = NULL;
    strl = strlen(str);

    /*strl must be greater than or equal to MIN_SIZE (6)*/
    if(strl>=MIN_SIZE) {

        /*convert basetime string in to integer*/
        t_t = atoi(str);
        /*convert in to local time using localtime() API from time.h*/
        tmp = localtime(&t_t);

        /*using strftime() API broken-down Time and Date format specified*/
        strftime(timestring,TIME_STR_SIZE,"%H:%M:%S",tmp);

        strftime(datestring,DATE_STR_SIZE,"%Y-%m-%d",tmp);

        snprintf(buf_date,buf_date_size,"%s",datestring);

        snprintf(buf_time,buf_time_size,"%s",timestring);
    }
}


/*
  Extract Code Dump information from the filename.
*/

int
extract_info (
 regex_t * regexst, const char * filename,struct core_dump_data* cd,int type)
{
    char value[SIZE_DATE_AND_TIME+1];
    int strsize = 0;
    int matchstatus;
    int result;
    char date_string[DATE_STR_SIZE]={0};
    char time_string[TIME_STR_SIZE]={0};
    /* contains the matches found. */
    regmatch_t match_found[TOTAL_INFO];

    if( type != TYPE_DAEMON && type != TYPE_KERNEL)
    {
        /* unknown type */
        return -1;
    }
    if (type == TYPE_DAEMON)
    {


        /* Get Daemon Name */
        result = getxattr(filename, "user.coredump.comm",cd->daemon_name,DEAMON_NAME_SIZE );

        if (result == -1)
        {
            return -1;
        }
        else if (result > DEAMON_NAME_SIZE)
        {
            cd->daemon_name[DEAMON_NAME_SIZE] = 0;
        }
        else
        {
            cd->daemon_name[result] = 0;
        }

        /* Get crash signal, to generate the crash message/reason */
        result = getxattr(filename, "user.coredump.signal", cd->crash_signal,SIGNAL_STR_SIZE);

        if (result == -1)
        {
            return -1;
        }
        else if (result > SIGNAL_STR_SIZE)
        {
            cd->crash_signal[SIGNAL_STR_SIZE] = 0;
        }
        else
        {
            cd->crash_signal[result] = 0;
        }

        /* Get PID, used as instance_id */
        result = getxattr(filename, "user.coredump.pid", cd->crash_instance_id, INSTANCE_ID_SIZE);

        if(result == -1)
        {
            return -1;
        }
        else if (result > INSTANCE_ID_SIZE)
        {
            cd->crash_instance_id[INSTANCE_ID_SIZE] = 0;
        }
        else
        {
            cd->crash_instance_id[result] = 0;
        }

        /* Get Date and Time*/

        result = getxattr(filename, "user.coredump.timestamp", value, SIZE_DATE_AND_TIME);

        if (result == -1)
        {
            return -1;
        }
        else if (result > SIZE_DATE_AND_TIME)
        {
            value[SIZE_DATE_AND_TIME] = 0;
        }
        else
        {
            value[result] = 0;
        }



        convert_to_date_and_time(date_string,time_string,DATE_STR_SIZE,TIME_STR_SIZE,value);

        strncpy(cd->crash_date,date_string,(DATE_STR_SIZE-1));
        cd->crash_date[DATE_STR_SIZE] = 0;

        strncpy(cd->crash_time,time_string,(TIME_STR_SIZE-1));
        cd->crash_time[TIME_STR_SIZE] = 0;

    }
    else if (type == TYPE_KERNEL)
    {
        matchstatus = regexec (regexst, filename, TOTAL_INFO, match_found, 0);
        if (matchstatus)
        {
            return -1;
        }

        /* Extract Date */
        if (match_found[1].rm_so == -1)
        {
            return -1;
        }
        strsize = match_found[1].rm_eo - match_found[1].rm_so;
        strncpy (cd->crash_date,(filename+match_found[1].rm_so),strsize);

        if(strsize != SRC_DATE_STR_LEN)
        {
            return -1;
        }

        /* Extract Time Stamp*/
        if (match_found[2].rm_so == -1)
        {
            return -1;
        }
        strsize = match_found[2].rm_eo - match_found[2].rm_so;
        strncpy (cd->crash_time,(filename+match_found[2].rm_so),strsize);
        if(strsize != SRC_TIME_STR_LEN)
        {
            return -1;
        }

        /* Format the Date */
        cd->crash_date[10] = 0;
        cd->crash_date[9] = cd->crash_date[7];
        cd->crash_date[8] = cd->crash_date[6];
        cd->crash_date[7] = '-';
        cd->crash_date[6] = cd->crash_date[5];
        cd->crash_date[5] = cd->crash_date[4];
        cd->crash_date[4] = '-';

        /* Format the time */
        cd->crash_time[8] = 0;
        cd->crash_time[7] = cd->crash_time[5];
        cd->crash_time[6] = cd->crash_time[4];
        cd->crash_time[5] = ':';
        cd->crash_time[4] = cd->crash_time[3];
        cd->crash_time[3] = cd->crash_time[2];
        cd->crash_time[2] = ':';
    }
    return 0;
}

/*
 * Function       : get_file_list
 * Responsibility : Generates lists of core files present for daemon and kernel
 * Parameters
 *                : type
 *                : globbuf
 *                : globpattern
 *                : daemon
 *
 * Returns        : 0 on success
 */

int
get_file_list(int type, glob_t* globbuf,
        const char* globpattern ,const char* daemon, const char* instance_id )
{
    char* corelocation = NULL;
    char location_buf[CORE_FILE_NAME];
    int rc=0;
    int locsize = 0;

    if( type != TYPE_DAEMON && type != TYPE_KERNEL)
    {
        /* unknown type */
        return -1;
    }

    if(type == TYPE_KERNEL)
        corelocation = KERNEL_CORE_PATH;
    else if (type == TYPE_DAEMON)
        corelocation = DAEMON_CORE_PATH;
    /* Form the GLOB pattern using the core dump location */
    if ( type == TYPE_KERNEL )
    {
        locsize = snprintf(location_buf ,CORE_FILE_NAME ,"%s/%s/%s",
                corelocation ,"kernel-core",globpattern);
    }
    else
    {
        if (( type == TYPE_DAEMON ) &&  daemon ) {
            /* Daemon name is specified in cli
               copy core-dump cli specify the daemon name*/
            if ( instance_id == NULL ) {
                /* user has not provided instance id */
                locsize = snprintf(location_buf,CORE_FILE_NAME,
                        "%s\\/%s\\.%s\\.%s",
                        corelocation,"core", daemon,globpattern);
            }
            else {
                /* user provided instance id */
                locsize = snprintf(location_buf,CORE_FILE_NAME,
                        "%s\\/%s\\.%s\\.*%s\\.%s",
                        corelocation,"core", daemon, instance_id, globpattern);
            }
        }
        else
        {
            /* Daemon name is unspecified in cli
               show core-dump doesn't specify the daemon name */
            locsize = snprintf(location_buf,CORE_FILE_NAME,
                    globpattern,corelocation);
        }
    }
    if(locsize > CORE_FILE_NAME)
    {
        if(type == TYPE_DAEMON)
        {
            VLOG_ERR("Invalid daemon name");
        }
        else if (type == TYPE_KERNEL)
        {
            VLOG_ERR("Invalid kernel name");
        }
        return -1;
    }

    /* Find the list of core dumps present in the core dump folder
       On Success :
       globbuf.gl_pathc will contain the number of core dumps found
       globbuf.gl_pathv will contain the core dump file names
       */
    rc = glob(location_buf,GLOB_BRACE,NULL,globbuf);

    /* globe returns error for nomatch . So ignore nomatch error */
    if ( rc == GLOB_NOMATCH )
    {
        rc = 0;
    }

    return rc;
}

/*
 * Function       : validate_cli_args
 * Responsibility : validates given cli argument with regular expression.
 * Parameters
 *                : arg - argument passed in cli
 *                : regex - regular expression to validate user input
 *
 * Returns        : 0 on success
 */

int
validate_cli_args(const char * arg , const char * regex)
{
    regex_t r;
    int rc = 0;
    const int n_matches = 10;
    regmatch_t m[n_matches];

    if (!( arg && regex ) )
        return 1;

    rc = regcomp(&r, regex , REG_EXTENDED|REG_NEWLINE);
    if ( rc )  {
        regfree (&r);
        return rc;
    }

    rc = regexec (&r,arg,n_matches, m, 0);
    regfree (&r);
    return rc;
}
