/* System SUPPORTABILITY CLI commands
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
* File: supportability_vty.c
*
* Purpose: To Install all Supportability CLI Commands
*/

#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/memory.h"
#include "dynamic-string.h"
#include "supportability_vty.h"
#include "show_events_vty.h"
#include "show_tech_vty.h"
#include "diag_dump_vty.h"
#include <sys/types.h>
#include <dirent.h>
#include "openvswitch/vlog.h"
#include "show_vlog_vty.h"
#include "supportability_utils.h"


VLOG_DEFINE_THIS_MODULE (vtysh_supportability_cli);


/*
 * Function           : install_show_vlog
 * Responsibility     : Install the show vlog command
 */
int
install_show_vlog()
{
    DIR * fd;
    struct dirent *fd1;
    char str[MAX_FILENAME_SIZE] = {0,};
    char *ptr = NULL, *cmd = NULL;
    int install = 0;
    fd = opendir(PID_DIRECTORY);
    if(fd == NULL) {
        VLOG_ERR("Directory open failure");
        return 1;
    }
    cmd = (char*)calloc(MAX_DAEMONS, MAX_FEATURE_NAME_SIZE);
    if(cmd == NULL) {
        VLOG_ERR("Memory allocation failure");
        return 1;
    }
    strncpy(cmd, VLOG_CMD, (MAX_VLOG_CMD-1));
    fd1 = readdir(fd);
    if(fd1 == NULL) {
        VLOG_ERR("Directory read failure");
        free(cmd);
        closedir(fd);
        return 1;
    }
    while(fd1)
    {
        strncpy(str, fd1->d_name, (MAX_FILENAME_SIZE-1));
        if(strstr(fd1->d_name, "ops"))
        {
            if(strstr(fd1->d_name, ".pid"))
            {
                strncpy(str, fd1->d_name, (MAX_FILENAME_SIZE-1));
                ptr = strtok(str, ".");
                strncat(cmd, ptr, ((MAX_VLOG_CMD - strlen(cmd)) - 1));
                strncat(cmd, "|", ((MAX_VLOG_CMD - strlen(cmd)) - 1));
                install = 1;
            }
        }
        fd1 = readdir(fd);
    }
    if(!install) {
       VLOG_ERR("Unable to read daemon list for VLOG");
       free(cmd);
       closedir(fd);
       return 1;
    }
    strncat(cmd, ")}", ((MAX_VLOG_CMD - strlen(cmd)) - 1));
    cli_platform_show_vlog_cmd.string = cmd;
    install_element (ENABLE_NODE, &cli_platform_show_vlog_cmd);
    closedir(fd);
    return 0;
}

/*
 * Function           : install_diag_dump
 * Responsibility     : Install the diag dump command
 */
int
install_diag_dump()
{
    yaml_parser_t parser;
    yaml_event_t ev_obj;
    yaml_event_t *token = &ev_obj;
    int found = 0, doc = 0;
    char *key = NULL, *help = NULL;
    char *cmd = (char*)calloc(MAX_FEATURES, MAX_FEATURE_NAME_SIZE);
    FILE *fh;
    int install = 0;
    if(cmd == NULL) {
        VLOG_ERR("Failed to calloc");
        return 1;
    }
    if (!yaml_parser_initialize(&parser)) {
        VLOG_ERR("YAML initialisation failure");
        free(cmd);
        return 1;
    }
    /* Keeping a hard limit of help string for 500 features
     * This has to be increased once 500 features are crossed.
     * Please note that if it crosses 500, there is no impact on
     * command , but help string will be juggled */
    help = (char*)calloc(MAX_FEATURES, MAX_HELP_SIZE);
    if(help == NULL) {
        VLOG_ERR("Failed to calloc");
        free(cmd);
        return 1;
    }

    fh = fopen(FEATURE_MAPPING_CONF, "r");
    if (fh == NULL) {
        VLOG_ERR("Failed to open file");
        free(cmd);
        free(help);
        return 1;
    }
    /* Now form the first part of cmd string */
    strncpy(cmd, "diag-dump (", (MAX_CMD_SIZE-1));
    /* Form the 1st part of help string */
    strncpy(help, DIAG_DUMP_STR, (MAX_FEATURE_HELP_SIZE-1));
    yaml_parser_set_input_file(&parser, fh);
    /* Loop through YAML file & populate the cmd string with tokens */
    while((key = get_yaml_tokens(&parser, &token, fh)) != NULL)
    {
        if((found) && (!strcmp_with_nullcheck(key, "feature_desc"))) {
            doc = 1;
            found = 0;
            yaml_event_delete(token);
            continue;
        }
        if(found) {
            /* Found a feature name append it to cmd */
            strncat(cmd, key, ((MAX_CMD_SIZE - strlen(cmd))-1));
            strncat(cmd, "|", ((MAX_CMD_SIZE - strlen(cmd))-1));
            install = 1;
        }
        if(doc) {
            /* populate help string for particular feature */
            strncat(help, key, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
            /* Append a newline after each help string */
            strncat(help, "\n", ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
            doc = 0;
        }
        if(!strcmp_with_nullcheck(key,"feature_name")) {
            found = 1;
        }
        yaml_event_delete(token);

    }
    if(!install) {
        /* This means we didn't find any feature in YAML file
           which possibly points to a corrupt YAML as well.
           We will skip installation in that case */
        VLOG_ERR("Unable to find any features in YAML file");
        free(cmd);
        free(help);
        yaml_parser_delete(&parser);
        fclose(fh);
        return 1;
    }
    /* Append ending part of cmd string */
    strncat(cmd, ") basic [FILENAME]", ((MAX_CMD_SIZE - strlen(cmd))-1));
    /* Now let cmd_element structure point to the new cmd & help string */
    vtysh_diag_dump_cmd.string = cmd;
    strncat(help, DIAG_DUMP_FEATURE_BASIC, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
    strncat(help, DIAG_DUMP_FEATURE_FILE, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
    vtysh_diag_dump_cmd.doc = help;
    install_element (ENABLE_NODE, &vtysh_diag_dump_cmd);
    yaml_event_delete(token);
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}

/*
 * Function           : install_show_tech
 * Responsibility     : Install the show tech command
 */
int
install_show_tech()
{
    yaml_parser_t parser;
    yaml_event_t ev_obj;
    yaml_event_t *token = &ev_obj;
    char *key = NULL,*help = NULL;
    int found = 0, install = 0;
    char *cmd = (char*)calloc(MAX_FEATURES, MAX_FEATURE_NAME_SIZE);
    char prev[MAX_HELP_SIZE] = {0,};
    char desc[MAX_HELP_SIZE] = {0,};
    if(cmd == NULL) {
        VLOG_ERR("Memory allocation failure");
        return 1;
    }
    /* Keeping a hard limit of help string for 500 features
     * This has to be increased once 500 features are crossed.
     * Please note that if it crosses 500, there is no impact on
     * command , but help string will be juggled */
    help = (char*)calloc(MAX_FEATURES, MAX_HELP_SIZE);
    if(help == NULL) {
        VLOG_ERR("Memory allocation failure");
        free(cmd);
        return 1;
    }
    if (!yaml_parser_initialize(&parser)) {
        VLOG_ERR("Failed to initialize parser");
        free(cmd);
        free(help);
        return 1;
    }
    char* filename="/etc/openswitch/supportability/ops_showtech.yaml";
    FILE* fh = fopen(filename, "r");
    if (fh == NULL) {
        VLOG_ERR("Failed to open file");
        free(cmd);
        free(help);
        return 1;
    }
    yaml_parser_set_input_file(&parser, fh);
    /* Copy the 1st part of cmd string */
    strncpy(cmd, "show tech (", (MAX_CMD_SIZE-1));
    /* Copy 1st part of help string */
    strncpy(help, cli_platform_show_tech_feature_cmd.doc,
       (MAX_FEATURE_HELP_SIZE-1));
    /* Loop through YAML file & append cmd string with feature name */
    while((key = get_yaml_tokens(&parser, &token, fh)) != NULL)
    {
        if(found) {
            strncat(cmd, key, ((MAX_CMD_SIZE - strlen(cmd))-1));
            strncat(cmd, "|", ((MAX_CMD_SIZE - strlen(cmd))-1));
            strncat(help, desc, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
            strncat(help, "\n", ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
            found = 0;
            install = 1;
        }
        if(!strcmp_with_nullcheck(key,"feature_name")) {
            found = 1;
            strncpy(desc, prev, (MAX_HELP_SIZE-1));
        }
        /* We have to save the key to prev as feature description
           comes before feature name in YAML file */
        strncpy(prev, key, (MAX_HELP_SIZE-1));
        yaml_event_delete(token);
    }
    if(!install) {
        /* This means we didn't find any feature in YAML file
           which possibly points to a corrupt YAML as well.
           We will skip installation in that case */
        VLOG_ERR("Unable to find any features in YAML file");
        free(cmd);
        free(help);
        yaml_parser_delete(&parser);
        fclose(fh);
        return 1;
    }
    /* Append last part of help string */
    strncat(help, SHOW_TECH_FILE_STR, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
    strncat(help, SHOW_TECH_FILENAME_STR, ((MAX_FEATURE_HELP_SIZE - strlen(help))-1));
    /* Append last part of cmd string */
    strncat(cmd, ") {localfile FILENAME}", ((MAX_CMD_SIZE - strlen(cmd))-1));
    /* Now let cmd element structure to point to newly formed cmd & help string */
    cli_platform_show_tech_feature_cmd.string = cmd;
    cli_platform_show_tech_feature_cmd.doc = help;
    install_element (ENABLE_NODE, &cli_platform_show_tech_feature_cmd);
    yaml_event_delete(token);
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}

/*
 * Function           : install_show_evnts
 * Responsibility     : Install the show events command
 */
int
install_show_evnts()
{
    yaml_parser_t parser;
    yaml_event_t ev_obj;
    yaml_event_t *token = &ev_obj;
    int found = 0, exit = 0, des = 0, found_des = 0;
    char *key = NULL, *help = NULL;
    char *cmd = (char*)calloc(MAX_EV_CATEGORIES, MAX_FEATURE_NAME_SIZE);
    int install = 0;
    if(cmd == NULL) {
        VLOG_ERR("Memory allocation failure");
        return 1;
    }
    if (!yaml_parser_initialize(&parser)) {
        VLOG_ERR("Failed to initialize parser");
        free(cmd);
        return 1;
    }
    FILE* fh = fopen(EVENTS_YAML_FILE, "r");
    if (fh == NULL) {
        VLOG_ERR("Failed to open file");
        free(cmd);
        return 1;
    }
    /* COpy 1st part of cmd string */
    strncpy(cmd, SHOW_EVENTS_CMD, (MAX_CMD_SIZE-1));
    yaml_parser_set_input_file(&parser, fh);
    /* Max no: of categories possible is 999 */
    help = (char*)calloc(MAX_EV_CATEGORIES, MAX_HELP_SIZE);
    if(help == NULL) {
        VLOG_ERR("Memory allocation failure");
        free(cmd);
        return 1;
    }
    /* Copy 1st part of help string */
    strncpy(help, cli_platform_show_events_cmd.doc,(MAX_EV_HELP_SIZE-1));
    /* Loop thru YAML file & append cmd string with cateogory name
     * & corrsponding help string */
    while((key = get_yaml_tokens(&parser, &token, fh)) != NULL)
    {
        if(found_des) {
            strncat(help, key, ((MAX_EV_HELP_SIZE - strlen(help))-1));
            strncat(help, "\n", ((MAX_EV_HELP_SIZE - strlen(help))-1));
            found_des = 0;
        }
        if(des) {
            found_des = 1;
            des = 0;
            }
        if(found) {
            strnlwr(key, strlen(key));
            strncat(cmd, key, ((MAX_CMD_SIZE - strlen(cmd))-1));
            strncat(cmd, "|", ((MAX_CMD_SIZE - strlen(cmd))-1));
            found = 0;
            des = 1;
            install = 1;
        }
        if(!strcmp_with_nullcheck(key, "event_definitions")) {
            exit = 1;
            break;
        }
        if(!strcmp_with_nullcheck(key, "event_category")) {
            found = 1;
        }
        yaml_event_delete(token);
    }
    if(!install) {
        /* This means we didn't find any Category in YAML file
           which possibly points to a corrupt YAML as well.
           We will skip installation in that case */
        VLOG_ERR("Unable to find any category in YAML file");
        free(cmd);
        free(help);
        yaml_parser_delete(&parser);
        return 1;
    }
    if(exit) {
        /* Append the command & form it properly */
        strncat(cmd, ")}", ((MAX_CMD_SIZE - strlen(cmd))-1));
        /* Now let cmd element structure point to newly formed help & cmd strings */
        cli_platform_show_events_cmd.string = cmd;
        cli_platform_show_events_cmd.doc = help;
        install_element (ENABLE_NODE, &cli_platform_show_events_cmd);
    }
    yaml_event_delete(token);
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}

/*
 * Function           : cli_pre_init
 * Responsibility     : Install the cli nodes
 */

void
cli_pre_init(void)
{
   /* Supportability Doesnt have any new node */

}

/*
 * Function           : cli_post_init
 * Responsibility     : Install the cli action routines
 *                      This function is common across all supportability cli
 *                      Install all supportability cli here
 */
void
cli_post_init()
{
  if(install_show_evnts()) {
      VLOG_ERR("show events command installation with CLI expand failed");
      return;
  }
  install_element (ENABLE_NODE, &cli_platform_show_tech_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_tech_file_cmd);
  if(install_show_tech()) {
      VLOG_ERR("show tech command installation with CLI expand failed");
      return;
  }

  if(install_diag_dump()) {
      VLOG_ERR("diag-dump command installation with CLI expand failed");
      return;
  }
  if(install_show_vlog()) {
      VLOG_ERR("show vlog command installation with CLI expand failed");
      return;
  }
  install_element (ENABLE_NODE, &cli_platform_show_tech_list_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_core_dump_cmd);

  install_element (ENABLE_NODE, &vtysh_diag_dump_list_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_vlog_config_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_vlog_config_list_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_vlog_feature_cmd);
  install_element (CONFIG_NODE, &cli_config_vlog_set_cmd);
}
