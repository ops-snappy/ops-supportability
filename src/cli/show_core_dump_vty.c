/* System SHOW_CORE DUMP CLI commands
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
* File: show_core_dump_vty.c
*
* Purpose: To Run Show Tech Commands from CLI
*/

#include <glob.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>

#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "show_core_dump_vty.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"

VLOG_DEFINE_THIS_MODULE (vtysh_show_core_dump_cli);

int
cli_show_core_dump(void)
{
   glob_t globbuf_daemon;
   glob_t globbuf_kernel;
   size_t i;

   regex_t regexst_daemon;
   regex_t regexst_kern;
   struct core_dump_data cd = {{0}};

   int header_p = 0;
   int num_of_core =0;



   /* Prepare the regular expression to parse information from the
      core files */

   if(0 != compile_corefile_pattern(&regexst_daemon, CORE_FILE_PATTERN))
   {
      vty_out(vty,"Invalid core dump pattern%s",VTY_NEWLINE);
      regfree(&regexst_daemon);
      return CMD_WARNING;
   }

   if(0 != compile_corefile_pattern(&regexst_kern, KERN_CORE_FILE_PATTERN))
   {
      vty_out(vty,"Invalid kernel core dump pattern%s",VTY_NEWLINE);
      regfree(&regexst_daemon);
      regfree(&regexst_kern);
      return CMD_WARNING;
   }
   /* Get File List for Daemon Cores
      On Success :
      globbuf_daemon.gl_pathc will contain the number of core dumps found
      globbuf_daemon.gl_pathv will contain the core dump file names
      */
   if(0 !=
         get_file_list(CORE_DUMP_CONFIG,TYPE_DAEMON,&globbuf_daemon,GB_PATTERN)
     )
   {
      /* Failed to locate Core Dump Files */

      regfree  (&regexst_daemon);
      regfree  (&regexst_kern);
      return CMD_WARNING;
   }

   /* For each core dump file present, extract the timestamp and daemon
    * information from the file name */
   for (i = 0; i < globbuf_daemon.gl_pathc;i++)
   {
      if(extract_info(
               &regexst_daemon,globbuf_daemon.gl_pathv[i],&cd,TYPE_DAEMON)
            != -1)
      {
         if(header_p == 0)
         {
            vty_out(vty
                  ,"===========================================================%s"
                  ,VTY_NEWLINE);
            vty_out(vty,"TimeStamp           | Daemon Name%s",VTY_NEWLINE);
            vty_out(vty
                  ,"===========================================================%s"
                  ,VTY_NEWLINE);
            header_p = 1;
         }
         num_of_core++;
         vty_out(vty,"%s %s   %s\n"
               ,cd.crash_date,cd.crash_time,cd.daemon_name);
      }
   }


   /* Get file list for kernel  cores
      On Success :
      globbuf_daemon.gl_pathc will contain the no of kernel core dumps
      globbuf_daemon.gl_pathv will contain the kernel core dump file names
      */
   if(get_file_list(KERNEL_DUMP_CONFIG,TYPE_KERNEL,
            &globbuf_kernel,KERN_GB_PATTERN)  != 0)
   {
      /* Failed to locate Core Dump Files */

      /* No Core Dump found so far, hence error out */
      if(header_p != 0)
      {
         /* Some Daemon Core Dumps are present, print the footer and return */
         vty_out(vty
               ,"===========================================================%s"
               ,VTY_NEWLINE);
         vty_out(vty
               ,"Total number of core dumps : %d%s",num_of_core
               ,VTY_NEWLINE);
         vty_out(vty
               ,"===========================================================%s"
               ,VTY_NEWLINE);

      }

      globfree (&globbuf_daemon);
      regfree  (&regexst_daemon);
      regfree  (&regexst_kern);

      return CMD_WARNING;
   }
   else
   {
      if(globbuf_kernel.gl_pathc  > 0)
      {
         if(extract_info(&regexst_kern,globbuf_kernel.gl_pathv[0],&cd
                  ,TYPE_KERNEL) != -1)
         {
            if(header_p == 0)
            {
              vty_out(vty
              ,"===========================================================%s"
              ,VTY_NEWLINE);
              vty_out(vty,"TimeStamp           | Daemon Name%s",VTY_NEWLINE);
              vty_out(vty
              ,"===========================================================%s"
              ,VTY_NEWLINE);
              header_p = 1;
            }
            num_of_core++;
            vty_out(vty,"%s %s   %s\n"
                  ,cd.crash_date,cd.crash_time,"kernel");
         }
      }
   }
   if(header_p)
   {
      vty_out(vty
            ,"===========================================================%s"
            ,VTY_NEWLINE);
      vty_out(vty
            ,"Total number of core dumps : %d%s",num_of_core
            ,VTY_NEWLINE);
      vty_out(vty
            ,"===========================================================%s"
            ,VTY_NEWLINE);
   }
   else
   {
      vty_out(vty,"No core dumps are present%s",VTY_NEWLINE);
   }

   globfree (&globbuf_daemon);
   globfree (&globbuf_kernel);
   regfree  (&regexst_daemon);
   regfree  (&regexst_kern);

   return CMD_SUCCESS;
}


/*
* Action routines for Show Core Dump
*/
DEFUN_NOLOCK (cli_platform_show_core_dump,
  cli_platform_show_core_dump_cmd,
  "show core-dump",
  SHOW_STR
  SHOW_CORE_DUMP_STR)
  {
    return cli_show_core_dump();
  }
