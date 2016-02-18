/* SHOW VLOG LIST CLI commands
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
 * File: show_vlog_vty.c
 *
 * Purpose: To Run Show Events Commands from CLI
 */

#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include <yaml.h>
#include "jsonrpc.h"
#include "feature_mapping.h"
#include <string.h>
#include "show_vlog_vty.h"

#define LIST_ARGC                0
#define ADD_TOK                  2
#define SET_ARGC                 3
#define MAX_SIZE                 100
#define POSITION                 MAX_SIZE+15
#define LIST_SIZE                20
#define LIST                     "vlog/list"
#define SET                      "vlog/set"
#define FEATURE                  "feature"
#define DAEMON                   "daemon"
#define FEATURE_REQUEST          1
#define DAEMON_REQUEST           2
#define SHOW_VLOG_CONFIG_REQUEST 3
#define SET_REQUEST              4
#define FREE(X)                  if(X) { free(X); X=NULL;}

VLOG_DEFINE_THIS_MODULE(vtysh_vloglist_cli);

static int
vtysh_vlog_interface_daemon(char *feature,char *daemon ,char **cmd_type ,
      int cmd_argc , int request);

static struct jsonrpc *
vtysh_vlog_connect_to_target(const char *target);

int
strcmp_with_nullcheck( const char *, const char *);

static struct feature *feature_head =NULL;

/*flag to check before parsing yaml file */
static int initialized =0;

static int flag = 0;

/*
 * Function       : vtysh_vlog_connect_to_target
 * Responsibility : populates jsonrpc client structure for a daemon
 * Parameters     : target  - daemon name
 * Returns        : jsonrpc client on sucess
 *                   NULL on Failure
 */

 static struct jsonrpc *
 vtysh_vlog_connect_to_target(const char *target)
 {
   struct jsonrpc *client=NULL;
   char *socket_name=NULL;
   int error=0;
   char * rundir = NULL;
   char *pidfile_name = NULL;
   pid_t pid = -1;

    if (!target) {
        VLOG_ERR("target is null");
        return NULL;
    }

    rundir = (char*)ovs_rundir();

    if(!rundir) {
         VLOG_ERR("rundir is null");
         return NULL;
    }

    if (target[0] != '/') {
         pidfile_name = xasprintf("%s/%s.pid", rundir ,target);
         if (!pidfile_name) {
            VLOG_ERR("pidfile_name is null");
            return NULL;
         }
         /*read the pid*/
         pid = read_pidfile(pidfile_name);
         if (pid < 0) {
            free(pidfile_name);
            return NULL;
         }

         free(pidfile_name);
         socket_name = xasprintf("%s/%s.%ld.ctl", rundir , target,
              (long int) pid);
         if (!socket_name) {
            VLOG_ERR("socket_name is null");
            return NULL;
         }
    }
    else {
         socket_name = xstrdup(target);
         if (!socket_name) {
            VLOG_ERR("socket_name is null, target:%s",target);
             return NULL;
         }
    }
        /*connects to a unixctl server socket*/
    error = unixctl_client_create(socket_name, &client);
    if (error) {
       VLOG_ERR("cannot connect to %s,error=%d", socket_name,error);
    }
    free(socket_name);
    return client;
 }

/*
 * Function       : vtysh_vlog_interface_daemon
 * Responsibility : send request to daemon using unixctl and get the result
 *                : and print on the console.
 * Parameters     : feature
 *                : daemon
 *                : cmd_type
 *                : cmd_argc
 * Returns        : 0 on success and non-zero on failure
 */

static int
vtysh_vlog_interface_daemon(char *feature,char *daemon ,char **cmd_type,
            int cmd_argc ,int request)
{
   struct jsonrpc *client = NULL;
   char *cmd_result = NULL;
   char *cmd_error = NULL;
   int rc=0;
   char **cmd_argv = cmd_type;
   int cmd_argcount = cmd_argc;
   char vlog_str[MAX_SIZE];
   int  opt=1;

   if(!(daemon && cmd_type)) {
      VLOG_ERR("invalid paramter daemon or command");
      return CMD_WARNING;
   }
     /*connect vtysh to the daemon*/
   client = vtysh_vlog_connect_to_target(daemon);
   if(!client) {
      return CMD_WARNING;
   }
   if(!strcmp_with_nullcheck(*cmd_type,LIST)) {
         /*cmd_type is vlog/list and copy vlog/list to vlog_str*/
       strncpy(vlog_str,LIST,sizeof(vlog_str));
   }else if(!strcmp_with_nullcheck(*(cmd_type+1),SET)) {
         /*cmd_type is vlog/set and copy vlog/set to vlog_str*/
        strncpy(vlog_str,SET,sizeof(vlog_str));
        opt = opt +1;
        cmd_argcount = cmd_argc - (opt);
        cmd_argv = cmd_argcount ? cmd_type + opt : NULL;
    }

    rc = unixctl_client_transact(client,vlog_str,cmd_argcount,cmd_argv,
            &cmd_result,&cmd_error);
   /*
    * unixctl_client_transact() api failure case
    * check cmd_error and rc value.
    * Nonzero rc failure case
    */
    if(rc) {
         VLOG_ERR("%s: transaction error:%s , rc =%d", daemon ,
                (cmd_error?cmd_error:"error") , rc);
         jsonrpc_close(client);
         FREE(cmd_result);
         FREE(cmd_error);
         return CMD_WARNING;
    }
    else {
            /* rc == 0 and cmd_result contains string is success*/
      switch(request)
      {
         case 1:  /*feature result*/

                  if(flag == 0) {
                     vty_out(vty,"========================================%s",
                        VTY_NEWLINE);
                     vty_out(vty,"Feature               Syslog     File%s",
                        VTY_NEWLINE);
                     vty_out(vty,"========================================%s",
                        VTY_NEWLINE);
                     vty_out(vty,"%-17.17s   %-17.17s%s",feature,
                        (cmd_result+POSITION),VTY_NEWLINE);
                     flag = 1;
                  }
                  break;

         case 2: /*daemon result*/
                  vty_out(vty,"======================================%s",
                        VTY_NEWLINE);
                  vty_out(vty,"Daemon              Syslog     File%s",
                        VTY_NEWLINE);
                  vty_out(vty,"======================================%s",
                        VTY_NEWLINE);
                  vty_out(vty,"%-17.17s %-17.17s%s",daemon,
                        (cmd_result+POSITION),VTY_NEWLINE);
                  break;

         case 3: /*show vlog result*/
                  /*flag == 0 means first time displays feature and deamon*/
                  if(flag == 0) {
                     vty_out(vty,"%-15.15s %-13.13s %-18.18s%s",feature,
                        daemon,(cmd_result+POSITION),VTY_NEWLINE);
                     flag = 1;
                     break;
                  }
                  if(flag == 1) {
                     /*flag == 1 means displays next daemons
                      * of corresponding feature*/
                     vty_out(vty,"                %-13.13s %-18.18s%s",
                        daemon,(cmd_result+POSITION),VTY_NEWLINE);
                  }
                  break;

         case 4:  break; /*SET REQUEST only for configuration changes by using
                           show vlog config feature/daemon to obtain the changes*/

         default: break;

      }
    }

    if(cmd_error) {
         VLOG_ERR("%s: server returned error:cmd_error str:%s,rc =%d",
                                 daemon ,cmd_error, rc);
         jsonrpc_close(client);
         FREE(cmd_result);
         FREE(cmd_error);
         return CMD_WARNING;
    }
    jsonrpc_close(client);
    FREE(cmd_result);
    FREE(cmd_error);
    return CMD_SUCCESS;
}


/* Function       :  cli_showvlog_feature
 * Responsibility :  Displays show vlog feature
 * Return         :  0 on Success 1 otherwise
 */

int
cli_showvlog_feature(const char *argv0, const char *argv1)
{

   static int rc = 0;
   int fun_argc = LIST_ARGC;
   struct feature *iter = feature_head;
   struct daemon *iter_daemon = NULL;
   int request;

   char *fun_argv = NULL;
   fun_argv = (char*)calloc(LIST_SIZE,sizeof(char));
   if(fun_argv == NULL)
   {
      VLOG_ERR("memory allocation failed");
      FREE(fun_argv);
      return CMD_WARNING;
   }
   strncpy(fun_argv,LIST,LIST_SIZE);

   if( argv0 == NULL ||argv1 == NULL) {
      FREE(fun_argv);
      return CMD_WARNING;
   }else if(!strcmp_with_nullcheck(argv0,FEATURE)) {
      /*argv0 matches feature request*/
      request = FEATURE_REQUEST;
      if(!initialized) {
         feature_head = get_feature_mapping();
         if(feature_head == NULL){
            vty_out(vty,"Error in retrieving the mapping of feature names \
                  to daemon names%s",VTY_NEWLINE);
            FREE(fun_argv);
            return CMD_WARNING;
         }
      }

         /* traverse linked list to find feature */
      for (iter=feature_head ; iter && strcmp_with_nullcheck(iter->name,argv1);
         iter = iter->next);

      if(iter) {
         VLOG_DBG("feature:%s",iter->name);
         iter_daemon = iter->p_daemon;
            /*traverse all daemons*/
         while(iter_daemon) {
            rc = vtysh_vlog_interface_daemon(iter->name,iter_daemon->name,
                  &fun_argv,fun_argc,request);
            if (!rc) {
               VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
            }else{
               VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
            }
            iter_daemon = iter_daemon->next;
         }
         flag = 0;
      }else{
         vty_out(vty,"Feature not present %s",VTY_NEWLINE);
         FREE(fun_argv);
         return CMD_WARNING;
      }
   }
   if(!strcmp_with_nullcheck(argv0,DAEMON)){
         request = DAEMON_REQUEST;
                     /*Directly given daemon name */
         rc = vtysh_vlog_interface_daemon(NULL,(char *)argv1,&fun_argv,
               fun_argc,request);

         if (!rc) {
             VLOG_DBG("daemon :%s , rc:%d",argv1,rc);
         } else {
             vty_out(vty,"Not able to communicate with daemon %s%s",argv1,
                   VTY_NEWLINE);
             FREE(fun_argv);
             return CMD_WARNING;
         }

      FREE(fun_argv);
      return CMD_SUCCESS;
   }

   FREE(fun_argv);
   return CMD_SUCCESS;
}

/* Function       :  cli_show_vlog_list
 * Responsibility :  Display features list
 * Return         :  0 on Success 1 otherwise
 */

int
cli_show_vlog_config_list(void)
{

   if(!initialized) {
      /* feature to daemon mapping */
      feature_head = get_feature_mapping();
      if(feature_head == NULL){
         vty_out(vty,"Error in retrieving the mapping of feature \
               names to daemon names%s",VTY_NEWLINE);
         return CMD_WARNING;
      }
   }
   struct feature *iter = feature_head;
   vty_out(vty,"=============================================%s",VTY_NEWLINE);
   vty_out(vty,"Features          Description%s",VTY_NEWLINE);
   vty_out(vty,"=============================================%s",VTY_NEWLINE);
   while(iter != NULL) {
       vty_out(vty,"%-17.17s %-50.50s %s",iter->name,iter->desc,VTY_NEWLINE);
       iter =iter->next;
   }
   return CMD_SUCCESS;
}


/* Function       :  cli_config_vlog_set
 * Responsibility :  configure feature loglevel
 * Return         :  0 on Success 1 otherwise
 */

int
cli_config_vlog_set(const char* type,
      const char *fd_name ,const char *destination,
      const char *level)
{
   static int rc =0;
   int len = 0;
   int fun_argc = SET_ARGC;
   char *fun_argv[SET_ARGC];
   char *name = NULL;
   int request = SET_REQUEST;
   struct feature *iter = feature_head;
   struct daemon *iter_daemon = NULL;
   int dest_len = strlen(destination);
   int level_len = strlen(level);

      /*destination should be syslog , file or all*/
   if( strncasecmp(destination,"syslog",dest_len) &&
         strncasecmp(destination,"file",dest_len) &&
         strncasecmp(destination,"all",dest_len)){
         vty_out(vty,"Invalid destination%s",VTY_NEWLINE);
         return CMD_WARNING;
   }

      /*if destination becomes all then copy "any" to
       * destination before passing to vlog/set*/
   if( !strncasecmp(destination,"all",dest_len)) {
         strncpy((char *)destination,"any",dest_len);
   }

   if( strncasecmp(level,"emer",level_len) &&
        strncasecmp(level,"err",level_len) &&
         strncasecmp(level,"warn",level_len) &&
          strncasecmp(level,"dbg",level_len) &&
           strncasecmp(level,"info",level_len) &&
            strncasecmp(level,"off",level_len)){
         vty_out(vty,"Invalid log level %s",VTY_NEWLINE);
         return CMD_WARNING;
   }

   if( fd_name == NULL) {
      return CMD_WARNING;
   } else {
      len = strlen(destination)+ strlen(level) + ADD_TOK;
      name = (char*)calloc(len,sizeof(char));
      strncat(name,destination,len);
      strncat(name,":",len);
      strncat(name,level,len);

      fun_argv[0] = (char *)fd_name;
      fun_argv[1] = SET;
      fun_argv[2] = name;
   }

   if(strcmp_with_nullcheck(type,FEATURE) == 0)
   {
      if(!initialized) {
         feature_head = get_feature_mapping();
         if(feature_head == NULL){
            vty_out(vty,"Error in retrieving the mapping of feature \
                  names to daemon names%s",VTY_NEWLINE);
            FREE(name);
            return CMD_WARNING;
         }
      }
      /*traverse linked list to find feature*/
      for (iter=feature_head ; iter && strcmp_with_nullcheck(iter->name,fd_name);
            iter = iter->next);

      if(iter != NULL) {
         VLOG_DBG("feature:%s",iter->name);
         iter_daemon = iter->p_daemon;
         while(iter_daemon) {
               /*Feature name is passing as an argument to
                * vtysh_vlog_interface_daemon*/
            rc = vtysh_vlog_interface_daemon(iter->name,iter_daemon->name,
                  fun_argv,fun_argc, request);
            if (!rc) {
               VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
            }
            iter_daemon = iter_daemon->next;
         }
      }
      else {
         vty_out(vty,"Feature not present %s",VTY_NEWLINE);
         FREE(name);
         return CMD_WARNING;
      }
   }
   else
   {
      /* daemon Name is directly given */
      rc = vtysh_vlog_interface_daemon(NULL,(char *)fd_name,fun_argv,
            fun_argc,request);
      if (!rc) {
         VLOG_DBG("daemon :%s , rc:%d",fd_name,rc);
      }
      else
      {
         vty_out(vty,"Not able to communicate with daemon %s%s",fd_name,
               VTY_NEWLINE);
         FREE(name);
         return CMD_WARNING;
      }
   }
   FREE(name);
   return CMD_SUCCESS;
}

/*CLI to configure the log settings of FILE or SYSLOG */

DEFUN_NOLOCK (cli_config_set_vlog,
      cli_config_vlog_set_cmd,
      "vlog (feature|daemon) NAME (syslog | file | all) " \
      "(emer | err | warn | info | dbg | off)",
      VLOG_CONFIG
      VLOG_CONFIG_FEATURE
      VLOG_CONFIG_DAEMON
      SHOW_VLOG_NAME
      VLOG_LOG_DEST_SYSLOG
      VLOG_LOG_DEST_FILE
      VLOG_LOG_DEST_ALL
      VLOG_LOG_LEVEL_EMER
      VLOG_LOG_LEVEL_ERR
      VLOG_LOG_LEVEL_WARN
      VLOG_LOG_LEVEL_INFO
      VLOG_LOG_LEVEL_DBG
      VLOG_LOG_LEVEL_OFF)
{
   return cli_config_vlog_set(argv[0],argv[1],argv[2],argv[3]);
}

/*Action routine for show vlog features list*/

DEFUN_NOLOCK (cli_platform_show_vlog_list,
   cli_platform_show_vlog_config_list_cmd,
   "show vlog config list",
   SHOW_STR
   SHOW_VLOG_STR
   SHOW_VLOG_CONFIG_STR
   SHOW_VLOG_LIST_FEATURE)
{
   return cli_show_vlog_config_list();
}

/* Function       :  cli_showvlog_config
 * Responsibility :  Display all features loglevels of
 *                   file & console destinations
 * Return         :  0 on Success 1 otherwise
 */
int
cli_showvlog_config()
{
   static int rc = 0;
   int fun_argc = LIST_ARGC;
   int request = SHOW_VLOG_CONFIG_REQUEST;
   struct feature *iter = feature_head;
   struct daemon *iter_daemon = NULL;
   char *fun_argv = NULL;
   if(!initialized) {
      feature_head = get_feature_mapping();
      if(feature_head == NULL){
         vty_out(vty,"Error in retrieving the mapping of feature \
               names to daemon names%s",VTY_NEWLINE);
         return CMD_WARNING;
      }
   }

   fun_argv= (char *)calloc(LIST_SIZE,sizeof(char));
   if(fun_argv == NULL) {
      VLOG_ERR("memory allocation failed");
      FREE(fun_argv);
      return CMD_WARNING;
   }
   strncpy(fun_argv,LIST,LIST_SIZE);

   vty_out(vty,"=================================================%s",
         VTY_NEWLINE);
   vty_out(vty,"Feature         Daemon          Syslog     File%s",
         VTY_NEWLINE);
   vty_out(vty,"=================================================%s",
         VTY_NEWLINE);

   /*traverse all features and displays corresponding daemons
    * log levels of file and syslog destination*/
   for(iter = feature_head ; iter != NULL ; iter = iter->next)
   {
      if(iter) {
         iter_daemon = iter->p_daemon;
         while(iter_daemon) {
            rc = vtysh_vlog_interface_daemon(iter->name,iter_daemon->name,
                  &fun_argv,fun_argc,request);
             if (!rc) {
                VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
             }
            iter_daemon = iter_daemon->next;
         }
         flag = 0;
      } else {
               vty_out(vty,"Failed to capture vlog information:%s %s",
                     iter->name, VTY_NEWLINE);
               FREE(fun_argv);
               return CMD_WARNING;
        }
   }
   FREE(fun_argv);
   return CMD_SUCCESS;
}

/*Action routine for show vlog features,log levels */

DEFUN_NOLOCK (cli_platform_show_vlog,
   cli_platform_show_vlog_config_cmd,
   "show vlog config",
   SHOW_STR
   SHOW_VLOG_STR
   SHOW_VLOG_CONFIG_STR)
{
    return cli_showvlog_config();
}

/*Action routine for show vlog feature*/

DEFUN_NOLOCK (cli_platform_showvlog_feature_list,
   cli_platform_show_vlog_feature_cmd,
   "show vlog config (feature | daemon) NAME",
   SHOW_STR
   SHOW_VLOG_STR
   SHOW_VLOG_CONFIG_STR
   SHOW_VLOG_FEATURE
   SHOW_VLOG_DAEMON
   SHOW_VLOG_NAME)
{
   return cli_showvlog_feature(argv[0],argv[1]);
}
