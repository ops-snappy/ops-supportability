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
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "openvswitch/vlog.h"
#include "diag_dump_vty.h"
#include "jsonrpc.h"

#define ARGC 2

VLOG_DEFINE_THIS_MODULE(vtysh_diag);


static int
vtysh_diag_dump_daemon( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , int fd );

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


static int vtysh_diag_check_crete_dir( char *dir);

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
#define STR_FORMAT "%-40.40s %-100.100s %s"
    struct feature* iter = head;
    vty_out(vty,"Diagnostic Dump Supported Features List%s",VTY_NEWLINE);
    vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
    vty_out(vty,STR_FORMAT,"Feature","Description",VTY_NEWLINE);
    vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
    while(iter != NULL) {
        vty_out(vty,STR_FORMAT,STR_NULL_CHK(iter->name),
                STR_NULL_CHK(iter->desc) ,VTY_NEWLINE);
        iter = iter->next;
    }

#undef  STR_FORMAT
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
        if(!strcmp_with_nullcheck(keystr[i],data)) {
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


/*
 * Function       : vtysh_diag_check_crete_dir
 * Responsibility : checks a directory ,
 *                  creates directory if directory is not present
 *
 * Parameters     : void
 * Returns        : 0 on sucess and nonzero on failure
 */

static int
vtysh_diag_check_crete_dir( char *dir)
{
    struct stat sb;
    int rc= 1;
    if (!dir){
        VLOG_ERR("invalid parameter dir");
        return 1;
    }

    rc =  stat(dir, &sb);


    if ( rc == 0 ) {
        /* dir already present*/
        if (S_ISDIR(sb.st_mode)) {
            return 0;
        }
        /* file already present, delete the file and create dir*/
        else if (S_ISREG(sb.st_mode)) {
            rc = unlink(dir );
            if (rc) {
                VLOG_ERR("unlink failed for %s",dir);
                return 1;
            }
            rc = mkdir (dir,DEFFILEMODE);
            return rc;
        }
        else {
            /* for all other case return error */
            VLOG_ERR("%s not a regular file or dir  ",dir);
            return 1;
        }
    }
    else{
        if ( rc == -1){
            if (ENOENT == errno) {
                rc = mkdir (dir,DEFFILEMODE);
                return rc;
            }
        }

    }

    return  rc;
}



/*
 * Function       : vty_diag_print_time
 * Responsibility : print current time to given string
 *
 * Parameters     : string and length of string
 * Returns        : 0 on sucess and nonzero on failure
 */

static int
vty_diag_print_time(char *time_str, unsigned  int size)
{
    time_t rawtime;
    char buf[MAX_TIME_STR_LEN] = {0};
    struct tm * timeinfo = NULL;
    char * asci_time = NULL;
    struct tm result;

    if (!time_str)
        return 1;

    if (!size)
        return 1;

    time(&rawtime);
    timeinfo = localtime_r(&rawtime,&result);
    if  (!timeinfo)
        return 1;

    asci_time=asctime_r(timeinfo,buf) ;
    if (!asci_time)
        return 1;

    snprintf(time_str,size,"Time : %s",asci_time);
    time_str[size-1]='\0';
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
            return  CMD_WARNING ;
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

#define  FEATURE_BEGIN\
            snprintf(write_buff,sizeof(write_buff),"%s\n",CLI_STR_EQUAL);\
            STR_SAFE(write_buff); \
            write(fd,write_buff,strlen(write_buff));

#define  FEATURE_END FEATURE_BEGIN


    int fun_argc=ARGC;
    struct feature* iter = feature_head;
    struct daemon* iter_daemon = NULL;
    unsigned int  daemon_count=0;
    unsigned int  daemon_resp =0;
    int rc = 0;
    int fd = -1;
    char  file_path[FILE_PATH_LEN_MAX] = {0};
    char *fun_argv[ARGC];
    char time_str[MAX_TIME_STR_LEN]={0};
    char write_buff[MAX_CLI_STR_LEN]={0};
    char err_buf[MAX_CLI_STR_LEN];


    fun_argv[1] = (char *)  argv[0];
    fun_argv[0] = DIAG_BASIC;

    if ( !initialized ) {
        rc = vtysh_diag_parse_diag_yml();
        if ( rc != 0 ) {
            vty_out(vty,"Fail to parse diagnostic yaml file. Check yaml file %s"
                    , VTY_NEWLINE);
            return  CMD_WARNING ;
        }
    }



    /* traverse linkedlist to find node */
    for (iter=feature_head ; iter && strcmp_with_nullcheck(iter->name,argv[0]);
            iter = iter->next);

    if (iter) {

        /* user provided filepath */
        if (argc >= 2){
            rc = vtysh_diag_check_crete_dir(DIAG_DUMP_DIR);
            if (rc) {
                vty_out (vty,"failed to check or create dir:%s%s",
                        DIAG_DUMP_DIR,VTY_NEWLINE);
                return CMD_WARNING;
            }

            if ( argv[1][0] == '/') {
                vty_out (vty,"please provide filename without /%s",VTY_NEWLINE);
                return CMD_WARNING;
            }

            if ( strlen(argv[1]) > USER_FILE_LEN_MAX ) {
                vty_out (vty,"please provide filename less than %d %s",
                        USER_FILE_LEN_MAX ,VTY_NEWLINE);
                return CMD_WARNING;
            }

            snprintf(file_path,sizeof(file_path),"%s/%s",DIAG_DUMP_DIR,argv[1]);
            STR_SAFE(file_path);

            fd = open (file_path, O_CREAT|O_EXCL|O_WRONLY);
            if ( !VALID_FD_CHECK (fd) ) {
                strerror_r (errno,err_buf,sizeof(err_buf));
                STR_SAFE(err_buf);


                VLOG_ERR("failed to open file error:%d,file:%s", errno, file_path);
                vty_out (vty, "failed to open file error:%s file:%s%s",
                        err_buf , file_path,VTY_NEWLINE);
                return CMD_WARNING;
            }
        }


        /* print header */
        rc = vty_diag_print_time(time_str,sizeof(time_str));
        if (rc) {
            strncpy (time_str,"",sizeof(time_str) );
            STR_SAFE(time_str);
        }

        if (VALID_FD_CHECK(fd)) {
            /*   print ==== line */
            FEATURE_BEGIN

            /* print time in header . time_str contains \n */
            snprintf(write_buff,sizeof(write_buff),"[Start] Feature %s %s",
                    argv[0], time_str);
            STR_SAFE(write_buff);
            write(fd,write_buff,strlen(write_buff));

            /*   print ==== line */
            FEATURE_BEGIN

        } else {

            vty_out ( vty,"%s%s", CLI_STR_EQUAL , VTY_NEWLINE );
            vty_out ( vty,"[Start] Feature %s %s %s",argv[0], time_str,
                    VTY_NEWLINE);
            vty_out ( vty,"%s%s", CLI_STR_EQUAL , VTY_NEWLINE );
        }

        VLOG_DBG("feature:%s , desc:%s",STR_NULL_CHK(iter->name),
                STR_NULL_CHK(iter->desc));
        iter_daemon = iter->p_daemon;
        while(iter_daemon) {
            daemon_count++;
            rc = vtysh_diag_dump_daemon(iter_daemon->name, fun_argv, fun_argc,
                    vty, fd );
            /*Count daemon responded */
            if (!rc) {
                VLOG_DBG("daemon :%s , rc:%d",iter_daemon->name,rc);
                daemon_resp++;
            }
            iter_daemon = iter_daemon->next;
        }


        if ( VALID_FD_CHECK (fd)) {
            /* print ===== */
            FEATURE_END

            snprintf(write_buff,sizeof(write_buff),
                    "[End] Feature %s\n",argv[0]);
            STR_SAFE(write_buff);
            write(fd,write_buff,strlen(write_buff));

            /* print ===== */
            FEATURE_END
        } else {
            vty_out ( vty,"%s%s",CLI_STR_EQUAL, VTY_NEWLINE );
            vty_out (vty, "[End] Feature %s %s",argv[0],VTY_NEWLINE);
            vty_out ( vty,"%s%s",CLI_STR_EQUAL, VTY_NEWLINE );
        }

    } else {
        VLOG_ERR("%s feature is not present",argv[0]);
        vty_out(vty,"%s feature is not present %s",argv[0], VTY_NEWLINE);
        CLOSE(fd);
        return CMD_WARNING;
    }


    if ( daemon_count  ==  daemon_resp ) {
        vty_out(vty,"Diagnostic dump captured for feature %s %s",
                argv[0],VTY_NEWLINE);
    }

    CLOSE(fd);
    return CMD_SUCCESS;

#undef FEATURE_BEGIN
#undef FEATURE_END
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
        free(socket_name);
        return NULL;
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
 *                : fd - file descriptor
 *                  prints on vtysh if fd is NULL
 *                  writes to file if  fd is valid
 * Returns        : 0 on sucess and nonzero on failure
 */

static int
vtysh_diag_dump_daemon( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , int fd)
{
#define  FEATURE_BEGIN\
            snprintf(write_buff,sizeof(write_buff),"%s\n",CLI_STR_HYPHEN);\
            STR_SAFE(write_buff);\
            write(fd,write_buff,strlen(write_buff));

#define  FEATURE_END FEATURE_BEGIN


    char write_buff[MAX_CLI_STR_LEN]={0};
    struct jsonrpc *client=NULL;
    char *cmd_result=NULL, *cmd_error=NULL;
    int rc=0;
    char  diag_cmd_str[DIAG_CMD_LEN_MAX] = {0};

    if  (!(daemon && cmd_type)) {
        VLOG_ERR("invalid parameter daemon or command ");
        return CMD_WARNING;
    }

    client = vtysh_diag_connect_to_target(daemon);
    if (!client) {
        VLOG_ERR("%s transaction error.client is null ", daemon);
        vty_out(vty,"failed to connect daemon %s %s",daemon,VTY_NEWLINE);
        return CMD_WARNING;
    }


    if ( !strcmp_with_nullcheck(*cmd_type,DIAG_BASIC)){
        strncpy(diag_cmd_str,DIAG_DUMP_BASIC_CMD ,  sizeof(diag_cmd_str) );
    }else{
        strncpy(diag_cmd_str,DIAG_DUMP_ADVANCED_CMD,sizeof(diag_cmd_str));
    }
    STR_SAFE(diag_cmd_str);

    rc = unixctl_client_transact(client, diag_cmd_str, cmd_argc , cmd_type,
            &cmd_result, &cmd_error);

   /*
   * unixctl_client_transact() api failure case
   *  check cmd_error and rc value.
   */


    /* Nonzero rc failure case */
    if (rc) {
        VLOG_ERR("%s: transaction error:%s , rc =%d", daemon ,
                STR_NULL_CHK(cmd_error)  , rc);
        jsonrpc_close(client);
        FREE(cmd_result);
        FREE(cmd_error);
        return CMD_WARNING;
    }

   /* rc == 0 and cmd_result contains string is success   */
    else if ( !VALID_FD_CHECK(fd)  && !strcmp_with_nullcheck(*cmd_type,DIAG_BASIC)) {
        /* basic ,  file not specified  =>  print on console */
        /* print if buffer contains output*/
        if (cmd_result) {
            vty_out ( vty,"%s%s",CLI_STR_HYPHEN, VTY_NEWLINE );
            vty_out (vty, "[Start] Daemon %s %s",daemon,VTY_NEWLINE);
            vty_out ( vty,"%s%s",CLI_STR_HYPHEN, VTY_NEWLINE );

            vty_out (vty,"%s %s",cmd_result,VTY_NEWLINE );

            vty_out ( vty,"%s%s",CLI_STR_HYPHEN, VTY_NEWLINE );
            vty_out (vty, "[End] Daemon %s %s",daemon,VTY_NEWLINE);
            vty_out ( vty,"%s%s",CLI_STR_HYPHEN, VTY_NEWLINE );

        }
    } else {
        /* all other case dump to file  */
        if ( VALID_FD_CHECK(fd) && !strcmp_with_nullcheck(*cmd_type,DIAG_BASIC)) {
            if ( cmd_result ) {

                /* print ------- */
                FEATURE_BEGIN

                snprintf(write_buff,sizeof(write_buff),
                        "[Start] Daemon %s\n",daemon);
                STR_SAFE(write_buff);
                write(fd,write_buff,strlen(write_buff));


                /* print ------- */
                FEATURE_BEGIN

                write(fd,cmd_result ,strlen(cmd_result));
                write(fd,"\n", 2);


                /* print ------- */
                FEATURE_END

                snprintf(write_buff,sizeof(write_buff),
                        "[End] Daemon %s\n",daemon );
                STR_SAFE(write_buff);
                write(fd,write_buff,strlen(write_buff));


                /* print ------- */
                FEATURE_END
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
        return CMD_WARNING;
    }


    jsonrpc_close(client);
    FREE(cmd_result);
    FREE(cmd_error);
    return CMD_SUCCESS;

#undef  FEATURE_BEGIN
#undef  FEATURE_END
}
