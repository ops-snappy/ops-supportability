/* System SHOW_TECH CLI commands
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
* File: show_tech_vty.c
*
* Purpose: To Run Show Tech Commands from CLI
*/

#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "show_tech_vty.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"
#include "showtech.h"
#include "vtysh/buffer.h"
#include <errno.h>
#include <unistd.h>
#include <time.h>

VLOG_DEFINE_THIS_MODULE (vtysh_show_tech_cli);

#define     READ_LINE_SIZE    256
#define     BASH_CAT_CMD      "bashcat"

/* Function       : exec_showtech_cmd
 * Resposibility  : Parse the cli command and execute as needed
 * Return         : CMD_SUCCESS on success CMD_WARNING otherwise
 */
int
exec_showtech_cmd(const char* cmd)
{
   const char* trim_cmd = cmd;
   FILE *catFile;
   char line[READ_LINE_SIZE];
   /* Input argument validation */
   if(cmd == NULL)
   {
     return CMD_WARNING;
   }

   /* Ignore whitespace at the beginning */
   while ( isspace((unsigned char)(*trim_cmd)))
   {
      trim_cmd++;
   }

   /* command contains only whitespaces :-( */
   if(*trim_cmd == 0)
   {
      vty_out(vty,"Invalid command%s",VTY_NEWLINE);
      return CMD_WARNING;
   }

   /* Check if the command is bashcat */
   if(strncmp_with_nullcheck(BASH_CAT_CMD,trim_cmd,strlen(BASH_CAT_CMD)) == 0)
   {
      trim_cmd += strlen(BASH_CAT_CMD);
      /* Ignore whitespace at the beginning  of file name*/
      while ( isspace((unsigned char)(*trim_cmd)))
      {
         trim_cmd++;
      }

      /* filename not given :-( */
      if(*trim_cmd == 0)
      {
         vty_out(vty,"Invalid file name%s",VTY_NEWLINE);
         return CMD_WARNING;
      }
      catFile = fopen(trim_cmd, "r");
      if(catFile == NULL)
      {
         /* Not able to read the file */
         vty_out(vty,"File %s is not readable%s",trim_cmd,VTY_NEWLINE);
         return CMD_WARNING;
      }

      while(fgets(line, READ_LINE_SIZE,catFile) != NULL)
      {
         /* check if the last character is \n, if so replace it, since \n
          * wont work with vty_out properly, we need to use VTY_NEWLINE */
         if(line[READ_LINE_SIZE-2] == '\n')
         {
            line[READ_LINE_SIZE-2] = 0;
            vty_out(vty,"%s%s",line,VTY_NEWLINE);
         }
         vty_out(vty,"%s",line);
      }
      vty_out(vty,"%s",VTY_NEWLINE);
      fclose(catFile);
      return CMD_SUCCESS;
   }
   else
   {
      return vtysh_execute(trim_cmd);
   }
}



/* Function       : print_failed_commands
 * Resposibility  : Print the list of cli commands that failed to execute as
 *                  well as to reset the failure status.
 * Return         : void
 */

void
print_failed_commands(struct feature* head)
{
   struct feature* iter = head;
   struct sub_feature* iter_sub = NULL;
   struct clicmds* iter_cli = NULL;
   int count = 1;
   while (iter != NULL)
   {
      iter_sub = iter->p_subfeature;

      while (iter_sub)
      {
         iter_cli = iter_sub->p_clicmds;
         while (iter_cli)
         {
            if(iter_cli->command_failed)
            {
               vty_out(vty,"  %d. %s%s",count++,iter_cli->command,VTY_NEWLINE);
               iter_cli->command_failed = 0;
            }
            iter_cli = iter_cli->next;
         }
         iter_sub = iter_sub->next;
      }
      iter = iter->next;
   }
}

/* Function       : cli_show_tech
 * Resposibility  : Display Show Tech Information
 * Return         : 0 on success 1 otherwise
 */

int
cli_show_tech(const char* feature,const char* sub_feature)
{
   struct feature       *iter,*head;
   struct sub_feature*  iter_sub = NULL;
   struct clicmds*      iter_cli = NULL;
   int                  valid_cmd = 0;
   int                  failure_count = 0;
   time_t               showtech_start, showtech_end;
   char timebuf[32];
   double showtech_exec_time = 0.0;
   /* Retrive the Show Tech Configuration Header */
   head = get_showtech_config(NULL);
   iter = head;
   /* If the Header is NULL, then report the Show Tech Configuration failure
    * and exit
    */
   if(iter == NULL)
   {
      VLOG_ERR("Failed to obtain Show Tech configuration");
      vty_out(vty, "Failed to obtain show tech configuration," \
            " please restore the configuration file using default file.%s" \
            "These files are located in /etc/openswitch/supportability/ %s"
            ,VTY_NEWLINE,VTY_NEWLINE);
      return CMD_WARNING;
   }

   time(&showtech_start);
   vty_out(vty,"====================================================%s"
         ,VTY_NEWLINE);
   vty_out(vty,"Show Tech executed on %s",
         ctime_r(&showtech_start,timebuf));
   vty_out(vty,"====================================================%s"
         ,VTY_NEWLINE);

   /* Show Tech All */
   if(feature == NULL)
   {
      /* Show Tech (all) is a valid command, hence update the flag */
      valid_cmd = 1;
      while (iter != NULL)
      {
         vty_out(vty,"====================================================%s"
               ,VTY_NEWLINE);
         vty_out(vty,"[Begin] Feature %s%s", iter->name, VTY_NEWLINE);
         vty_out(vty,"====================================================%s%s"
               ,VTY_NEWLINE,VTY_NEWLINE);

         iter_sub = iter->p_subfeature;

         while (iter_sub)
         {
            /* CLI Commands are grouped under SubFeature, In order to support
             * configurations without any subfeature, we internally add a
             * dummy subfeature and set the flag is_dummy to true.
             * Checking the Dummy Flag before printing Sub Feature Information
             */
            if(!iter_sub->is_dummy)
            {
               vty_out(vty,
                     "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                     ,VTY_NEWLINE);
               vty_out(vty,
                     "[Begin] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
               vty_out(vty,
                  "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                  ,VTY_NEWLINE,VTY_NEWLINE);
            }
            iter_cli = iter_sub->p_clicmds;
            while (iter_cli)
            {
               vty_out(vty,"%s*********************************%s"
                     ,VTY_NEWLINE,VTY_NEWLINE);
               vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
               vty_out(vty,"*********************************%s"
                     ,VTY_NEWLINE);
               if (exec_showtech_cmd(iter_cli->command) != CMD_SUCCESS)
               {
                  failure_count++;
                  iter_cli->command_failed = 1;
                  vty_out(vty,"%s*********************************%s"
                        ,VTY_NEWLINE,VTY_NEWLINE);
                  vty_out(vty,
                        "Command %s failed to execute%s",
                        iter_cli->command,VTY_NEWLINE);
                  vty_out(vty,"*********************************%s"
                        ,VTY_NEWLINE);

               }
               iter_cli = iter_cli->next;
            }
            if(!iter_sub->is_dummy)
            {
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                     ,VTY_NEWLINE);
               vty_out(vty,"[End] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                     ,VTY_NEWLINE,VTY_NEWLINE);
            }

            iter_sub = iter_sub->next;
         }
         vty_out(vty,"====================================================%s"
               ,VTY_NEWLINE);
         vty_out(vty,"[End] Feature %s%s", iter->name, VTY_NEWLINE);
         vty_out(vty,"====================================================%s%s"
               ,VTY_NEWLINE,VTY_NEWLINE);


         iter = iter->next;
      }
   }
   /* Show Tech All Ends Here */
   else
   {
      /* Show Tech Feature */
      if(sub_feature == NULL)
      {
         while (iter != NULL)
         {
            if(strcmp_with_nullcheck(iter->name,feature))
            {
               /* Run till the matched feature */
               iter = iter->next;
               continue;
            }
            valid_cmd = 1;
            vty_out(vty,"====================================================%s"
                  ,VTY_NEWLINE);
            vty_out(vty,"[Begin] Feature %s%s", iter->name, VTY_NEWLINE);
            vty_out(vty,"====================================================%s%s"
                  ,VTY_NEWLINE,VTY_NEWLINE);
            iter_sub = iter->p_subfeature;

            while (iter_sub)
            {
               /* CLI Commands are grouped under SubFeature, In order to support
                * configurations without any subfeature, we internally add a
                * dummy subfeature and set the flag is_dummy to true.
                * Checking the Dummy Flag before printing Sub Feature Information
                */
               if(!iter_sub->is_dummy)
               {
                  vty_out(vty,
                     "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                     ,VTY_NEWLINE);
                  vty_out(vty,
                     "[Begin] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
                  vty_out(vty,
                  "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                   ,VTY_NEWLINE,VTY_NEWLINE);
               }
               iter_cli = iter_sub->p_clicmds;
               while (iter_cli)
               {
                  vty_out(vty,"%s*********************************%s"
                        ,VTY_NEWLINE,VTY_NEWLINE);
                  vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
                  vty_out(vty,"*********************************%s"
                        ,VTY_NEWLINE);
                  if (exec_showtech_cmd(iter_cli->command) != CMD_SUCCESS)
                  {
                     failure_count++;
                     iter_cli->command_failed = 1;
                     vty_out(vty,"%s*********************************%s"
                           ,VTY_NEWLINE,VTY_NEWLINE);
                     vty_out(vty,"Command %s failed to execute%s",
                           iter_cli->command,VTY_NEWLINE);
                     vty_out(vty,"*********************************%s"
                           ,VTY_NEWLINE);
                  }
                  iter_cli = iter_cli->next;
               }
               if(!iter_sub->is_dummy)
               {
                  vty_out(vty,
                     "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                     ,VTY_NEWLINE);
                  vty_out(vty,
                        "[End] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
                  vty_out(vty,
                  "= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                  ,VTY_NEWLINE,VTY_NEWLINE);
               }
               iter_sub = iter_sub->next;
            }

            if(valid_cmd)
            {
               vty_out(vty,
                     "====================================================%s"
                     ,VTY_NEWLINE);
               vty_out(vty,"[End] Feature %s%s", iter->name, VTY_NEWLINE);
               vty_out(vty,
                     "====================================================%s%s"
                     ,VTY_NEWLINE,VTY_NEWLINE);

               /* Required Feature Commands are Executed */
               break;
            }
            iter = iter->next;
         }
      }
      /* Show Tech Feature Ends here */

      /* Show Tech Sub Feature */
      else
      {
         while (iter != NULL)
         {
            if(strcmp_with_nullcheck(iter->name,feature))
            {
               /* Run till the matched feature */
               iter = iter->next;
               continue;
            }
            iter_sub = iter->p_subfeature;

            while (iter_sub)
            {
               /* CLI Commands are grouped under SubFeature, In order to support
                * configurations without any subfeature, we internally add a
                * dummy subfeature and set the flag is_dummy to true.
                * Checking the Dummy Flag before printing Sub Feature Information
                */
               if(iter_sub->is_dummy || strcmp_with_nullcheck(iter_sub->name,sub_feature))
               {
                  /* Run till the matched sub feature */
                  iter_sub = iter_sub->next;
                  continue;
               }
               if(valid_cmd == 0)
               {
                  valid_cmd = 1;
               }

               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                     ,VTY_NEWLINE);
               vty_out(vty,"[Begin] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
               vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                     ,VTY_NEWLINE,VTY_NEWLINE);
               iter_cli = iter_sub->p_clicmds;
               while (iter_cli)
               {
                  vty_out(vty,"%s*********************************%s"
                        ,VTY_NEWLINE,VTY_NEWLINE);
                  vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
                  vty_out(vty,"*********************************%s"
                        ,VTY_NEWLINE);
                  if (exec_showtech_cmd(iter_cli->command) != CMD_SUCCESS)
                  {
                     failure_count++;
                     iter_cli->command_failed = 1;
                     vty_out(vty,"%s*********************************%s"
                           ,VTY_NEWLINE,VTY_NEWLINE);
                     vty_out(vty,"Command %s failed to execute%s", iter_cli->command,VTY_NEWLINE);
                     vty_out(vty,"*********************************%s"
                           ,VTY_NEWLINE);
                  }
                  iter_cli = iter_cli->next;
               }
               if(valid_cmd)
               {
                  vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
                        ,VTY_NEWLINE);
                  vty_out(vty,"[End] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
                  vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
                        ,VTY_NEWLINE,VTY_NEWLINE);

                  /* Required Sub Feature Commands are Executed */
                  break;
               }

               iter_sub = iter_sub->next;
            }
            if(valid_cmd)
            {
               /* Required Feature Commands are Executed */
               break;
            }
            iter = iter->next;
         }
      }
      /* Show Tech SubFeature Ends here */


      /* The Feature or Sub Feature Specified is not valid/found */
      if(valid_cmd == 0)
      {
         if(sub_feature == NULL)
         {
            /* Feature Specific Command */
            vty_out(vty,"Feature %s is not supported%s",feature,VTY_NEWLINE);
         }
         else
         {
            /* Sub Feature Specific Command */
            vty_out(vty,"Sub Feature %s is not supported%s",sub_feature,VTY_NEWLINE);
         }
         return CMD_SUCCESS;
      }
   }

   /* If Any Show Command has failed, then report the Failures
    * ToDo : It would be nice to print the name of the commands that failed in
    *        addition to the count
    */
   if(failure_count)
   {
      vty_out(vty,"%s====================================================%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
      if(failure_count  == 1)
      {
         vty_out(vty,"%d show tech command failed to execute%s",
               failure_count,VTY_NEWLINE);
         vty_out(vty,"====================================================%s"
               ,VTY_NEWLINE);
         vty_out(vty,"Failed command:%s",VTY_NEWLINE);
      }
      else
      {
         vty_out(vty,"%d show tech commands failed to execute%s",
               failure_count,VTY_NEWLINE);
         vty_out(vty,"====================================================%s"
               ,VTY_NEWLINE);
         vty_out(vty,"Failed commands:%s",VTY_NEWLINE);
      }
      print_failed_commands(head);
   }
   else
   {
      vty_out(vty,"%s====================================================%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
      vty_out(vty,"Show Tech commands executed successfully%s",VTY_NEWLINE);
      vty_out(vty,"====================================================%s"
            ,VTY_NEWLINE);
   }
   time(&showtech_end);
   showtech_exec_time = difftime(showtech_end,showtech_start);
   /* Display only if show tech execution taken more than a second */
   if(showtech_exec_time > 0)
   {
      vty_out(vty,"Show Tech took %10.6f seconds for execution%s",
            showtech_exec_time,VTY_NEWLINE);
   }

   return CMD_SUCCESS;
}


/* Function       : cli_show_tech_list
 * Resposibility  : Display Supported Show Tech Features
 * Return         : 0 on success -1 otherwise
 */

int
cli_show_tech_list(void)
{
  struct feature* iter;
#ifdef _ST_SUBFEATURE_ENABLED
  struct sub_feature* iter_sub = NULL;
#endif /* _ST_SUBFEATURE_ENABLED */
  iter = get_showtech_config(NULL);

  /* If the Header is NULL, then report the Show Tech Configuration failure
   * and exit
   */

  if(iter == NULL)
  {
    VLOG_ERR("Failed to obtain Show Tech configuration");
    vty_out(vty, "Failed to obtain show tech configuration," \
            " please restore the configuration file using default file.%s" \
            "These files are located in /etc/openswitch/supportability/ %s"
            ,VTY_NEWLINE,VTY_NEWLINE);

    return CMD_SUCCESS;
  }
  vty_out(vty,"Show Tech Supported Features List %s",VTY_NEWLINE);
  vty_out(vty,"------------------------------------------------------------%s"
  ,VTY_NEWLINE);
#ifdef _ST_SUBFEATURE_ENABLED
  vty_out(vty,"Feature  SubFeature        Desc%s",VTY_NEWLINE);
#else
  vty_out(vty,"Feature                    Desc%s",VTY_NEWLINE);
#endif /* _ST_SUBFEATURE_ENABLED */
  vty_out(vty,"------------------------------------------------------------%s"
  ,VTY_NEWLINE);
  while (iter != NULL)
  {
    vty_out(vty,"%-27.26s%s%s", iter->name, iter->desc,VTY_NEWLINE);
#ifdef _ST_SUBFEATURE_ENABLED
    vty_out(vty,"%s",VTY_NEWLINE);
    iter_sub = iter->p_subfeature;

    while (iter_sub)
    {
      if(!iter_sub->is_dummy)
      {
        vty_out(vty,"         %-18.17s%s%s%s", iter_sub->name, iter_sub->desc
        ,VTY_NEWLINE,VTY_NEWLINE);
      }
      iter_sub = iter_sub->next;
    }
#endif /* _ST_SUBFEATURE_ENABLED */
    iter = iter->next;
  }
  return CMD_SUCCESS;
}

#define CHARBUF 160


/* Function       : cli_show_tech_file
 * Resposibility  : Execute Show Tech and store them in the given file
 * Return         : CMD_SUCCESS on success CMD_WARNING on failure
 */

int
cli_show_tech_file(const char* fname,const char* feature,int force)
{
    int fd = -1;
    char filename[CHARBUF] = "/tmp/";
    char errorbuf[CHARBUF];
    //int bckup_fd = -1;
    int bckup_type = 0;
    char* outputbuf;

    if(fname == NULL)
    {
      vty_out(vty, "Output file name not found%s",VTY_NEWLINE);
      return CMD_WARNING;
    }
    if(strlen(fname) >(CHARBUF-11))
    {
      vty_out(vty, "File name should be less then 150 characters%s",VTY_NEWLINE);
      return CMD_WARNING;
    }
    /* Add File Name to the /tmp/ path */
    strcat(filename,fname);
    if (force)
    /* User requested to overwrite existing file */
    {
      fd = open(filename,O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP);
    }
    else
    {
      fd = open(filename,O_CREAT|O_EXCL|O_WRONLY,S_IRUSR|S_IWUSR|S_IRGRP);
    }
    if( fd < 0)
    {
       switch(errno)
       {
         case EEXIST :
         {
            vty_out(vty,"%s already exists, please give different name " \
                  "or use force option to overwrite existing file%s"
                  ,filename,VTY_NEWLINE);
            break;
         }
         case ENOENT :
         case EACCES :
         {

            vty_out(vty,"Permission denied: can't create file %s%s"
                  ,filename,VTY_NEWLINE);
            break;
         }
         default :
         {
            vty_out(vty, "File creation error(%d) : %s%s",
               errno,strerror_r(errno,errorbuf,(CHARBUF-1)),VTY_NEWLINE);
            break;
         }
       }
       return CMD_WARNING;
    }

    bckup_type = vty->type;
    vty->type = VTY_FILE;

    cli_show_tech(feature,NULL);

    if(vty->obuf == NULL)
    {
      vty_out(vty,"Show Tech execution failed%s",VTY_NEWLINE);
      close(fd);
      return CMD_WARNING;
    }
    outputbuf = buffer_getstr(vty->obuf);
    vty->type = bckup_type;
    /* Reset the Buffer */
    buffer_reset(vty->obuf);
    if(outputbuf)
    {
      write(fd, outputbuf,strlen(outputbuf));
      free(outputbuf);
      vty_out(vty,"Show Tech output stored in file %s%s",filename,VTY_NEWLINE);
    }
    else
    {
      vty_out(vty,"Show Tech execution failed%s",VTY_NEWLINE);
    }

    close(fd);
    return CMD_SUCCESS;
}

/*
* Action routines for Show Tech CLIs
*/
DEFUN_NOLOCK (cli_platform_show_tech,
  cli_platform_show_tech_cmd,
  "show tech",
  SHOW_STR
  SHOW_TECH_STR)
  {
    return cli_show_tech(NULL,NULL);
  }


/*
* Action routines for Show Tech List
*/
DEFUN_NOLOCK (cli_platform_show_tech_list,
  cli_platform_show_tech_list_cmd,
  "show tech list",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_LIST_STR)
  {
    return cli_show_tech_list();
  }



/*
* Action routines for Show Tech localfile
*/
DEFUN_NOLOCK (cli_platform_show_tech_file,
  cli_platform_show_tech_file_cmd,
  "show tech localfile FILENAME",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_FILE_STR
  SHOW_TECH_FILENAME_STR)
  {
      return cli_show_tech_file(argv[0],NULL,0);
  }


/*
* Action routines for Show Tech feature filename
*/
DEFUN_NOLOCK (cli_platform_show_tech_feature_file,
  cli_platform_show_tech_feature_file_cmd,
  "show tech FEATURE localfile FILENAME",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_FEATURE_STR
  SHOW_TECH_FILE_STR
  SHOW_TECH_FILENAME_STR)
  {
      return cli_show_tech_file(argv[1],argv[0],0);
  }




/*
* Action routines for Show Tech localfile
*/
DEFUN_NOLOCK (cli_platform_show_tech_file_force,
  cli_platform_show_tech_file_force_cmd,
  "show tech localfile FILENAME force",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_FILE_STR
  SHOW_TECH_FILENAME_STR
  SHOW_TECH_FILE_FORCE_STR)
  {
      return cli_show_tech_file(argv[0],NULL,1);

  }


/*
* Action routines for Show Tech feature filename
*/
DEFUN_NOLOCK (cli_platform_show_tech_feature_file_force,
  cli_platform_show_tech_feature_file_force_cmd,
  "show tech FEATURE localfile FILENAME force",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_FEATURE_STR
  SHOW_TECH_FILE_STR
  SHOW_TECH_FILENAME_STR
  SHOW_TECH_FILE_FORCE_STR)
  {
      return cli_show_tech_file(argv[1],argv[0],1);
  }


/*
* Action routines for Show Tech List
*/

#ifdef _ST_SUBFEATURE_ENABLED

DEFUN_NOLOCK (cli_platform_show_tech_feature,
  cli_platform_show_tech_feature_cmd,
  "show tech FEATURE [SUBFEATURE]",
  SHOW_STR
  SHOW_TECH_STR
  SHOW_TECH_FEATURE_STR
  SHOW_TECH_SUB_FEATURE_STR)
  {
    if(argc > 1)
    {
      /* Optional Sub Feature Provided */
      return cli_show_tech(argv[0],argv[1]);
    }
    else
    {
      return cli_show_tech(argv[0],NULL);
    }
  }

#else

DEFUN_NOLOCK (cli_platform_show_tech_feature,
  cli_platform_show_tech_feature_cmd,
  "show tech FEATURE",
  SHOW_STR
  SHOW_TECH_STR)
  {
      if(argv[1] != NULL) {
       return cli_show_tech_file(argv[1],argv[0],0);
      }
      else {
          return cli_show_tech(argv[0],NULL);
      }
  }

#endif /* _ST_SUBFEATURE_ENABLED */
