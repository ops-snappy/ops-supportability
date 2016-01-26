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

VLOG_DEFINE_THIS_MODULE (vtysh_show_tech_cli);

/* Function       : cli_show_tech
 * Resposibility  : Display Show Tech Information
 * Return         : 0 on success 1 otherwise
 */

int
cli_show_tech(const char* feature,const char* sub_feature)
{
  struct feature*      iter;
  struct sub_feature*  iter_sub = NULL;
  struct clicmds*      iter_cli = NULL;
  int                  valid_cmd = 0;
  int                  failure_count = 0;

  /* Retrive the Show Tech Configuration Header */
  iter = get_showtech_config(NULL);

  /* If the Header is NULL, then report the Show Tech Configuration failure
   * and exit
   */
  if(iter == NULL)
  {
    VLOG_ERR("Failed to obtain Show Tech configuration");
    vty_out(vty, "Failed to obtain Show Tech configuration%s",VTY_NEWLINE);
    return CMD_SUCCESS;
  }


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
          vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
          ,VTY_NEWLINE);
          vty_out(vty,"[Begin] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
          vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
          ,VTY_NEWLINE,VTY_NEWLINE);
        }
        iter_cli = iter_sub->p_clicmds;
        while (iter_cli)
        {
          vty_out(vty,"%s---------------------------------%s"
          ,VTY_NEWLINE,VTY_NEWLINE);
          vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
          vty_out(vty,"---------------------------------%s"
          ,VTY_NEWLINE);
          if (vtysh_execute(iter_cli->command) != CMD_SUCCESS)
          {
            failure_count++;
            vty_out(vty,"%s---------------------------------%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
            vty_out(vty,"Command %s failed to execute%s", iter_cli->command,VTY_NEWLINE);
            vty_out(vty,"---------------------------------%s"
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
            vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s"
            ,VTY_NEWLINE);
            vty_out(vty,"[Begin] Sub Feature %s%s", iter_sub->name,VTY_NEWLINE);
            vty_out(vty,"= = = = = = = = = = = = = = = = = = = = = = = = = = =%s%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
          }
          iter_cli = iter_sub->p_clicmds;
          while (iter_cli)
          {
            vty_out(vty,"%s---------------------------------%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
            vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
            vty_out(vty,"---------------------------------%s"
            ,VTY_NEWLINE);
            if (vtysh_execute(iter_cli->command) != CMD_SUCCESS)
            {
              failure_count++;
              vty_out(vty,"%s---------------------------------%s"
              ,VTY_NEWLINE,VTY_NEWLINE);
              vty_out(vty,"Command %s failed to execute%s", iter_cli->command,VTY_NEWLINE);
              vty_out(vty,"---------------------------------%s"
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

        if(valid_cmd)
        {
          vty_out(vty,"====================================================%s"
          ,VTY_NEWLINE);
          vty_out(vty,"[End] Feature %s%s", iter->name, VTY_NEWLINE);
          vty_out(vty,"====================================================%s%s"
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
            vty_out(vty,"%s---------------------------------%s"
            ,VTY_NEWLINE,VTY_NEWLINE);
            vty_out(vty,"Command : %s%s", iter_cli->command,VTY_NEWLINE);
            vty_out(vty,"---------------------------------%s"
            ,VTY_NEWLINE);
            if (vtysh_execute(iter_cli->command) != CMD_SUCCESS)
            {
              failure_count++;
              vty_out(vty,"%s---------------------------------%s"
              ,VTY_NEWLINE,VTY_NEWLINE);
              vty_out(vty,"Command %s failed to execute%s", iter_cli->command,VTY_NEWLINE);
              vty_out(vty,"---------------------------------%s"
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

    vty_out(vty,"%d show tech commands failed to execute%s",
    failure_count,VTY_NEWLINE);
    vty_out(vty,"====================================================%s"
    ,VTY_NEWLINE);
  }
  else
  {
    vty_out(vty,"%s====================================================%s"
    ,VTY_NEWLINE,VTY_NEWLINE);
    vty_out(vty,"Show Tech commands executed successfully%s",VTY_NEWLINE);
    vty_out(vty,"====================================================%s"
    ,VTY_NEWLINE);
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
    vty_out(vty, "Failed to obtain Show Tech configuration%s",VTY_NEWLINE);
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
        vty_out(vty,"         %-18.17s%s%s\n", iter_sub->name, iter_sub->desc
        ,VTY_NEWLINE);
      }
      iter_sub = iter_sub->next;
    }
#endif /* _ST_SUBFEATURE_ENABLED */
    iter = iter->next;
  }
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
  SHOW_TECH_STR
  SHOW_TECH_FEATURE_STR)
  {
    return cli_show_tech(argv[0],NULL);
  }

#endif /* _ST_SUBFEATURE_ENABLED */
