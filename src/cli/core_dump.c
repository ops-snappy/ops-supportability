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
  Extract Code Dump information from the filename.
 */

int
extract_info (
 regex_t * regexst, const char * filename,struct core_dump_data* cd,int type)
{
   int strsize = 0;
   int matchstatus;
   /* contains the matches found. */
   regmatch_t match_found[TOTAL_INFO];

   if( type != TYPE_DAEMON && type != TYPE_KERNEL)
   {
      /* unknown type */
      return -1;
   }

   matchstatus = regexec (regexst, filename, TOTAL_INFO, match_found, 0);
   if (matchstatus)
   {
      return -1;
   }

   if (type == TYPE_DAEMON)
   {
      /* Extract Daemon Name */
      if (match_found[1].rm_so == -1)
      {
         return -1;
      }
      strsize = match_found[1].rm_eo - match_found[1].rm_so;
      strncpy (cd->daemon_name,(filename+match_found[1].rm_so),strsize);
      cd->daemon_name[strsize] = 0;

      /* Extract Index */
      if (match_found[2].rm_so == -1)
      {
         return -1;
      }
      strsize = match_found[2].rm_eo - match_found[2].rm_so;
      strncpy (cd->crash_index,(filename+match_found[2].rm_so),strsize);
      cd->crash_index[strsize] = 0;

      /* Extract Date */
      if (match_found[3].rm_so == -1)
      {
         return -1;
      }
      strsize = match_found[3].rm_eo - match_found[3].rm_so;
      strncpy (cd->crash_date,(filename+match_found[3].rm_so),strsize);

      if(strsize != SRC_DATE_STR_LEN)
      {
         return -1;
      }

      /* Extract Time Stamp*/
      if (match_found[4].rm_so == -1)
      {
         return -1;
      }
      strsize = match_found[4].rm_eo - match_found[4].rm_so;
      strncpy (cd->crash_time,(filename+match_found[4].rm_so),strsize);
      if(strsize != SRC_TIME_STR_LEN)
      {
         return -1;
      }
   }
   else if (type == TYPE_KERNEL)
   {
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
   return 0;
}

int
get_file_list(const char* filepath,int type,
   glob_t* globbuf, const char* globpattern )
{
   char* config_token  = NULL;
   FILE *config_fp = NULL;
   char config_line[CORE_LOC_CONFIG];
   char* corelocation = NULL;
   char location_buf[CORE_FILE_NAME];

   if( type != TYPE_DAEMON && type != TYPE_KERNEL)
   {
      /* unknown type */
      return -1;
   }
   /* Open daemon core dump configuration file to find the daemon core
      dump location */
   config_fp = fopen(filepath,"r");
   if (config_fp == NULL)
   {
      /* Failed to open the file */
      if (type == TYPE_KERNEL)
      {
         vty_out(vty,"Unable to read kernel core dump config file%s"
               ,VTY_NEWLINE );
      }
      else if (type == TYPE_DAEMON)
      {
         vty_out(vty,"Unable to read daemon core dump config file%s"
               ,VTY_NEWLINE );
      }
      return CMD_WARNING;
   }

   /* Find the core dump location from the configuration file */
   while ( fgets (config_line , CORE_LOC_CONFIG , config_fp) != NULL )
   {
      if(type == TYPE_DAEMON)
      {
         corelocation = strstr(config_line,"corepath");
      }
      else if (type == TYPE_KERNEL)
      {
         corelocation = strstr(config_line,"path");
      }
      /* Config Line Reached */
      if(corelocation)
      {
         /* String contains the corepath key, find its value */
         if(type == TYPE_DAEMON)
         {
            config_token = strtok(config_line, "=");
         }
         else if (type == TYPE_KERNEL)
         {
            config_token = strtok(config_line, " ");
         }

         if (config_token == NULL)
         {
            corelocation = NULL;
            break;
         }
         else
         {

            if(type == TYPE_DAEMON)
            {
               /* Verify that the first token contains the key */
               corelocation = strstr(config_token,"corepath");
               if(corelocation == NULL)
               {
                  break;
               }
               config_token = strtok(NULL, "=");
            }
            else if (type == TYPE_KERNEL)
            {
               /* Verify that the first token contains the key */
               corelocation = strstr(config_token,"path");
               if(corelocation == NULL)
               {
                  break;
               }
               config_token = strtok(NULL, " ");
            }

            if (config_token == NULL)
            {
               corelocation = NULL;
               break;
            }
            else
            {
               /* Configuration Read Successfully, trim the data */
               corelocation = trim_white_space(config_token);
            }
         }
         /* Break the loop since the location is found */
         break;
      }
   }


   /* Core dump location configuration is not found in the
      configuration file */
   if(corelocation == NULL)
   {
      if(type == TYPE_DAEMON)
      {
         vty_out(vty,"Invalid daemon core dump config file%s"
               ,VTY_NEWLINE);
      }
      else if (type == TYPE_KERNEL)
      {
         vty_out(vty,"Invalid kernel core dump config file%s"
               ,VTY_NEWLINE );
      }

      fclose(config_fp);
      config_fp = NULL;
      return -1;
   }


   /* Form the GLOB pattern using the core dump location */
   int locsize = 0;
   locsize = snprintf(location_buf,CORE_FILE_NAME,globpattern,corelocation);
   if(locsize > CORE_FILE_NAME)
   {
      if(type == TYPE_DAEMON)
      {
         vty_out(vty,"Invalid daemon core dump config file%s"
               ,VTY_NEWLINE);
      }
      else if (type == TYPE_KERNEL)
      {
         vty_out(vty,"Invalid kernel core dump config file%s"
               ,VTY_NEWLINE);
      }

      fclose(config_fp);
      config_fp = NULL;
      return -1;
   }

   /* Find the list of core dumps present in the core dump folder
      On Success :
      globbuf.gl_pathc will contain the number of core dumps found
      globbuf.gl_pathv will contain the core dump file names
      */
   glob(location_buf,GLOB_BRACE,NULL,globbuf);
   fclose(config_fp);
   config_fp = NULL;
   return 0;
}
