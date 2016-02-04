/* Diagnostic dump CLI commands file
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
 * File: diag_dump_vty.c
 *
 * Purpose:  To add diag-dump CLI display commands.
 */



#include <stdio.h>
#include <yaml.h>
#include "openvswitch/vlog.h"
#include "diag_dump_vty.h"
#include "jsonrpc.h"

#define ARGC 2
#define FREE(X)\
        if(X) { free (X); X = NULL; }

#define FCLOSE(X)\
        if(X) { fclose(X); X = NULL; }


VLOG_DEFINE_THIS_MODULE(vtysh_diag);


static int
vtysh_diag_dump_daemon( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , char* file_path );

static struct jsonrpc *
vtysh_diag_connect_to_target(const char *target);

static int
vtysh_diag_parse_diag_yml(void);

static int
vtysh_diag_check_key(const char *data);

static struct daemon*
vtysh_diag_add_daemon(struct daemon* afternode,
        const char* daemon);

static void
vtysh_diag_list_features (struct feature* head ,  struct vty *vty);

static int
vtysh_diag_add_feature_desc(struct feature* element,
        const char* feature_desc);

static struct feature*
vtysh_diag_add_feature(struct feature* afternode,
        const char* feature_name);


const char* keystr[MAX_NUM_KEYS] = {
    "values",
    "feature_name",
    "feature_desc",
    "daemon",
};

static struct feature* feature_head;
static char initialized=0; /* flag to check before parseing yaml file */

/*
 * Function       : vtysh_diag_add_feature
 * Responsibility : create and add a node in linked list . New node contains
 *                  feature name string
 *
 * Parameters
 *                : feature_name - name of feature
 *                : afternode - new nodes next pointer will point to this
 *
 * Returns        : NULL on failure
 *                  Pointer to new node on sucess
 */

static struct feature*
vtysh_diag_add_feature(struct feature* afternode,
        const char* feature_name)
{
    struct feature* elem = NULL;
    elem = (struct feature*)calloc(1,sizeof(struct feature));
    if(elem == NULL) {
        VLOG_ERR("Memory Allocation Failure\n");
        return NULL;
    }
    elem->name = strdup(feature_name);
    if(afternode != NULL) {
        elem->next = afternode->next;
        afternode->next = elem;
    }
    return elem;
}

/*
 * Function       : vtysh_diag_add_feature_desc
 * Responsibility : add a feature node in linked list
 * Parameters
 *                : feature_name - name of feature
 *                : afternode - new nodes next pointer will point to this node
 *
 * Returns        : 0 on scucess
 *                  nonzero on failure
 */


static int
vtysh_diag_add_feature_desc(struct feature* element,
        const char* feature_desc)
{
    if(element == NULL) {
        return 1;
    }
    element->desc = strdup(feature_desc);
    return 0;
}


/*
 * Function       : vtysh_diag_list_features
 * Responsibility : display list of feature name with description
 * Parameters
 *                :  head - head pointer of linked list
 *                :  vty
 *
 * Returns        : void
 */

static void
vtysh_diag_list_features (struct feature* head ,  struct vty *vty)
{
    struct feature* iter = head;
    vty_out(vty,"Diagnostic Dump Supported Features List%s",VTY_NEWLINE);
    while(iter != NULL) {
        vty_out(vty,"%s\t\t\t%s %s",iter->name,iter->desc ,VTY_NEWLINE);
        iter = iter->next;
    }
}

/*
 * Function       : vtysh_diag_add_daemon
 * Responsibility : add daemon name in feature linked list
 * Parameters
 *                : afternode - new nodes next pointer will point to this node
 *                : daemon - name of daemon
 * Returns        : void
 */

static struct daemon*
vtysh_diag_add_daemon(struct daemon* afternode,
        const char* daemon)
{
    struct daemon* elem = NULL;
    elem = (struct daemon*)calloc(1,sizeof(struct daemon));
    if(elem == NULL) {
        VLOG_ERR("Memory Allocation Failure\n");
        return NULL;
    }
    elem->name = strdup(daemon);
    if(afternode != NULL) {
        elem->next = afternode->next;
        afternode->next = elem;
    }
    return elem;

}

/*
 * Function       : vtysh_diag_check_key
 * Responsibility : helper function to parse yaml file
 * Parameters
 *                : data
 * Returns        : integer
 */


static int
vtysh_diag_check_key(const char *data)
{
    int i = FEATURE_NAME;
    for( i = FEATURE_NAME; i < MAX_NUM_KEYS; i++) {
        if(!strcmp(keystr[i],data)) {
            return i;
        }
    }
    /* Data didn't match any of the keywords, hence it should be value */
    return VALUE;
}

/*
 * Function       : vtysh_diag_parse_diag_yml
 * Responsibility : parse diagnostic feature to daemon mapping config file
 *                  and store in linkedlist
 * Parameters     : void
 * Returns        : 0 on sucess and nonzero on failure
 */

static int
vtysh_diag_parse_diag_yml(void)
{
    FILE *fh=NULL;
    yaml_parser_t parser;
    yaml_event_t  event;
    int event_value = 0;
    int current_state = 0;
    struct feature*   curr_feature    = NULL;
    struct daemon*    curr_daemon     = NULL;
    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        VLOG_ERR("Failed to initialize parser!");
        return 1;
    }
    fh = fopen(DIAG_DUMP_CONF, "r");
    if(fh == NULL) {
        VLOG_ERR("Failed to open file :%s",DIAG_DUMP_CONF);
        yaml_parser_delete(&parser);
        return 1;
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fh);

    /* START new code */
    if (!yaml_parser_parse(&parser, &event)) {
        VLOG_ERR("Parser error %d", parser.error);
        yaml_parser_delete(&parser);
        fclose(fh);
        return 1;
    }

    while(event.type != YAML_STREAM_END_EVENT){

        if(event.type == YAML_SCALAR_EVENT) {
            event_value =
                vtysh_diag_check_key ((const char*) event.data.scalar.value);
            switch(event_value) {
                case VALUE:
                    {
                        switch(current_state) {
                            case FEATURE_NAME:
                                {
                                    curr_daemon      = NULL;
                                    curr_feature = vtysh_diag_add_feature(
                                            curr_feature, (const char*)
                                            event.data.scalar.value);
                                    if(!feature_head)
                                    {
                                        feature_head = curr_feature;
                                    }
                                    break;
                                }
                            case FEATURE_DESC:
                                {
                                    vtysh_diag_add_feature_desc(curr_feature,
                                            (const char*)
                                            event.data.scalar.value);
                                    break;
                                }
                            case DAEMON:
                                {
                                    curr_daemon = vtysh_diag_add_daemon(
                                            curr_daemon, (const char*)
                                            event.data.scalar.value);
                                    if(curr_feature->p_daemon == NULL)
                                    {
                                        curr_feature->p_daemon = curr_daemon;
                                    }
                                    break;
                                }

                        }
                        break;
                    }
                case FEATURE_NAME:
                    {
                        current_state = FEATURE_NAME;

                        break;
                    }
                case FEATURE_DESC:
                    {
                        current_state = FEATURE_DESC;

                        break;
                    }
                case DAEMON:
                    {
                        current_state = DAEMON;
                        break;
                    }

            }

        }
        if(event.type != YAML_STREAM_END_EVENT)
            yaml_event_delete(&event);
        if (!yaml_parser_parse(&parser, &event)) {
            VLOG_ERR("Parser error %d\n", parser.error);
            yaml_parser_delete(&parser);
            fclose(fh);
        }
    }
    yaml_event_delete(&event);

    /* Cleanup */
    yaml_parser_delete(&parser);
    fclose(fh);
    initialized=1;
    return 0;
}


DEFUN (vtysh_diag_dump_list_show,
        vtysh_diag_dump_list_cmd,
        "diag-dump list",
        DIAG_DUMP_STR
        DIAG_DUMP_LIST_STR
      )
{

    int rc=0;
    if ( !initialized ) {
        rc = vtysh_diag_parse_diag_yml();
        if ( rc != 0 ) {
            vty_out(vty,"Fail to parse diagnostic yaml file. Check yaml file %s"
                    , VTY_NEWLINE);
            return  rc ;
        }
    }
    vtysh_diag_list_features(feature_head,vty);
    return CMD_SUCCESS;
}

DEFUN (vtysh_diag_dump_show,
        vtysh_diag_dump_cmd,
        "diag-dump (FEATURE_NAME) basic [FILENAME]",
        DIAG_DUMP_STR
        DIAG_DUMP_FEATURE
        DIAG_DUMP_FEATURE_BASIC
        DIAG_DUMP_FEATURE_FILE
      )
{
    int fun_argc=ARGC;
    char file_path[FILE_PATH_LEN_MAX]="";
    char *fun_argv[ARGC];
    struct feature* iter = feature_head;
    struct daemon* iter_daemon = NULL;
    unsigned int  daemon_count=0;
    unsigned int  daemon_resp =0;
    int rc = 0;

    fun_argv[1] = (char *)  argv[0];
    fun_argv[0] = DIAG_BASIC;

    if ( !initialized ) {
        rc = vtysh_diag_parse_diag_yml();
        if ( rc != 0 ) {
            vty_out(vty,"Fail to parse diagnostic yaml file. Check yaml file %s"
                    , VTY_NEWLINE);
            return  rc ;
        }
    }


    /* traverse linkedlist to find node */
    for (iter=feature_head ;   iter && strcmp(iter->name,argv[0]) ;
            iter = iter->next);
    if (iter) {
        if (argc >= 2)
            strncpy(file_path,argv[1],sizeof(file_path));

        VLOG_DBG("feature:%s , desc:%s",iter->name,iter->desc);
        iter_daemon = iter->p_daemon;
        while(iter_daemon) {
            daemon_count++;
            rc = vtysh_diag_dump_daemon(iter_daemon->name, fun_argv, fun_argc,
                    vty,file_path);
            if (!rc) {
                VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
                daemon_resp++;
            }
            iter_daemon = iter_daemon->next;
        }
    } else {
        VLOG_ERR("%s feature is not present",argv[0]);
        vty_out(vty,"%s feature is not present %s",argv[0], VTY_NEWLINE);
        return CMD_WARNING;
    }
    if ( daemon_count  ==  daemon_resp ) {
        vty_out(vty,"Diagnostic dump captured for feature %s %s",
                argv[0],VTY_NEWLINE);
    }
    return CMD_SUCCESS;
}

/*
 * Function       : vtysh_diag_connect_to_target
 * Responsibility : populates jsonrpc client structure for a daemon
 * Parameters     : target  - daemon name
 * Returns        : jsonrpc client on sucess
 *                  NULL on failure
 *
 */

static struct jsonrpc *
vtysh_diag_connect_to_target(const char *target)
{
    struct jsonrpc *client=NULL;
    char *socket_name=NULL;
    int error=0;
    char * rundir = NULL;
    char *pidfile_name = NULL;
    pid_t pid=-1;

    if (!target)
    {
        VLOG_ERR("target is null");
        return NULL;
    }

    rundir = (char*) ovs_rundir();
    if (!rundir)
    {
        VLOG_ERR("rundir is null");
        return NULL;
    }

    if (target[0] != '/') {

        pidfile_name = xasprintf("%s/%s.pid", rundir ,target);
        if (!pidfile_name) {
            VLOG_ERR("pidfile_name is null");
            return NULL;
        }

        pid = read_pidfile(pidfile_name);
        if (pid < 0) {
            VLOG_ERR("cannot read pidfile :%s", pidfile_name);
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

    } else {
        socket_name = xstrdup(target);
        if (!socket_name) {
            VLOG_ERR("socket_name is null, target:%s",target);
            return NULL;
        }
    }

    error = unixctl_client_create(socket_name, &client);
    if (error) {
        VLOG_ERR("cannot connect to %s,error=%d", socket_name,error);
    }
    free(socket_name);

    return client;
}

/*
 * Function       : vtysh_diag_dump_daemon
 * Responsibility : send request to dump diagnostic info using unixctl and
 *                  print result to console or file .
 * Parameters
 *                : daemon
 *                : cmd_type - basic  or advanced
 *                : cmd_argc
 *                : vty
 *                : file_path - path of file to capture diagnostic information
 * Returns        : 0 on sucess and nonzero on failure
 */

static int
vtysh_diag_dump_daemon( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , char* file_path )
{
    struct jsonrpc *client=NULL;
    char *cmd_result=NULL, *cmd_error=NULL;
    int rc=0;
    FILE *fp=NULL;
    char  diag_cmd_str[100];

    if  (!(daemon && cmd_type)) {
        VLOG_ERR("invalid parameter daemon or command ");
        return CMD_WARNING;
    }
    /* user specified file path then open a file to write it */
    if ( strlen(file_path)){
        fp = fopen(file_path ,"a");
        if (!fp) {
            VLOG_ERR("failed to open file :%s",file_path);
            return CMD_WARNING;
        }
    }
    client = vtysh_diag_connect_to_target(daemon);
    if (!client) {
        VLOG_ERR("%s transaction error.client is null ", daemon);
        vty_out(vty,"failed to connect daemon %s %s",daemon,VTY_NEWLINE);
        FCLOSE(fp);
        return CMD_WARNING;
    }

    if ( !strcmp(*cmd_type,DIAG_BASIC)){
        strncpy(diag_cmd_str,DIAG_DUMP_BASIC_CMD ,  sizeof(diag_cmd_str) );
    }else{
        strncpy(diag_cmd_str,DIAG_DUMP_ADVANCED_CMD,sizeof(diag_cmd_str));
    }

    rc = unixctl_client_transact(client, diag_cmd_str, 2 , cmd_type,
            &cmd_result, &cmd_error);

   /*
   * unixctl_client_transact() api failure case
   *  check cmd_error and rc value.
   */


    /* Nonzero rc failure case */
    if (rc) {
        VLOG_ERR("%s: transaction error:%s , rc =%d", daemon ,
                (cmd_error?cmd_error:"error") , rc);
        jsonrpc_close(client);
        FREE(cmd_result);
        FREE(cmd_error);
        FCLOSE(fp);
        return CMD_WARNING;
    }

   /* rc == 0 and cmd_result contains string is success   */
    else if ( ( strlen(file_path) == 0)  && !strcmp(*cmd_type,DIAG_BASIC)) {
        /* basic ,  file not specified  =>  print on console */
        /* print if buffer contains output*/
        if (cmd_result) {
            vty_out(vty,"Diagnostic  dump for daemon %s %s",daemon,VTY_NEWLINE);
            vty_out(vty,"%s %s",cmd_result,VTY_NEWLINE );
        }
    } else {
        /* all other case dump to file  */
        if (cmd_result) {
            if (fp) {
                fprintf(fp,"%s\n",cmd_result);
            }
        }
    }

    /* if cmd_error contains string then failure case */

    if (cmd_error) {
        VLOG_ERR("%s: server returned error:rc=%d,error str:%s",
                daemon,rc,cmd_error);
        jsonrpc_close(client);
        FREE(cmd_result);
        FREE(cmd_error);
        FCLOSE(fp);
        return CMD_WARNING;
    }


    jsonrpc_close(client);
    FREE(cmd_result);
    FREE(cmd_error);
    FCLOSE(fp);
    return CMD_SUCCESS;
}