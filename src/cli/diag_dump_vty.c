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
#include <pthread.h>
#include <signal.h>
#include "supportability_utils.h"
#define ARGC 2
#define  ERR_STR\
    "Feature to daemon mapping failed. Unable to retrieve the daemon name."

VLOG_DEFINE_THIS_MODULE(vtysh_diag);


struct jsonrpc *client=NULL;
struct vty *gVty = NULL;

static int
vtysh_diag_dump_daemon( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , int fd );

static int
vtysh_diag_dump_create_thread( char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , int fd );

static void *
vtysh_diag_dump_thread( void* arg);

static struct jsonrpc *
vtysh_diag_connect_to_target(const char *target);


static void
vtysh_diag_list_features (struct feature* head ,  struct vty *vty);


static int vtysh_diag_check_crete_dir( char *dir);

static struct feature* feature_head;
static char initialized=0; /* flag to check before parseing yaml file */


void diagdump_user_interrupt_handler(int );

/* diag dump global var should be intialized to zero */
bool gDiagDumpUserInterrupt        = FALSE ;
bool gDiagDumpUserInterruptAlarm   = FALSE ;
bool gDiagDumpCommandFailed        = FALSE ;
bool gDiagDumpThreadCancelled      = FALSE ;

pthread_mutex_t gDiagDumpwaitMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t gDiagDumpWaitCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t gDiagDumpCleanupMutex = PTHREAD_MUTEX_INITIALIZER;


/* Function       : dd_mutex_lock
 * Resposibility  : wrapper for mutex lock sys call, to log failed state
 * Return         : NULL
 */
int dd_mutex_lock(pthread_mutex_t *mutex)
{
    int rc = -1 ;
    rc = pthread_mutex_lock(mutex);
    if(rc)
    {
        VLOG_ERR("diag dump:mutex lock failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : dd_mutex_unlock
 * Resposibility  : wrapper for mutex unlock sys call, to log failed state
 * Return         : NULL
 */
int dd_mutex_unlock(pthread_mutex_t *mutex)
{
    int rc = -1 ;
    rc = pthread_mutex_unlock(mutex);
    if(rc)
    {
        VLOG_ERR("diag dump:mutex unlock failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : dd_alarm
 * Resposibility  : wrapper for alarm sys call, to log failed state
 * Return         : NULL
 */
unsigned dd_alarm(unsigned seconds)
{
    int rc = -1 ;
    rc = alarm(seconds);
    if(rc)
    {
        VLOG_ERR("diag dump:alarm failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : diagdump_thread_cleanup_handler
 * Resposibility  : to clean the cmd thread , currently it will close the
 *                  connection
 * Return         : NULL
 */
void diagdump_thread_cleanup_handler(void *arg )
{
   jsonrpc_close(client);
   client = NULL;
   dd_mutex_unlock( &gDiagDumpCleanupMutex );
}

/* Function       : vtysh_diag_dump_thread
 * Resposibility  : call the function which execute the diag dump
 * Return         : 0 on success and nonzero on failure
 */
void *
vtysh_diag_dump_thread(void * argc)
{
   thread_arg arg = *(thread_arg *) argc;
   char *daemon;
   char ** cmd_type;
   int cmd_argc;
   struct vty * vty;
   int fd ;
   daemon = arg.daemon;
   cmd_type = arg.cmd_type;
   cmd_argc = arg.cmd_argc;
   vty = arg.vty;
   fd = arg.fd;
   if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
   {
       VLOG_ERR("diag dump - unable to set cancel state of new thread");
       return (void *) 0;
   }
   if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL))
   {
       VLOG_ERR("diag dump - unable to set cancel type of new thread");
       return (void *) 0;
   }
   pthread_cleanup_push(diagdump_thread_cleanup_handler, NULL);
   dd_mutex_lock( &gDiagDumpCleanupMutex );

   if(vtysh_diag_dump_daemon( daemon , cmd_type ,cmd_argc ,vty ,fd ))
       gDiagDumpCommandFailed = TRUE ;

   dd_mutex_lock( &gDiagDumpwaitMutex );
   if(pthread_cond_signal( &gDiagDumpWaitCond ))
   {
       VLOG_ERR("diag dump - unable to enable signal main thread");
   }
   dd_mutex_unlock( &gDiagDumpwaitMutex );
   pthread_cleanup_pop(0);
   dd_mutex_unlock( &gDiagDumpCleanupMutex );
   return (void *) 0;
}

/*
 * Function       : vtysh_diag_dump_create_thread
 * Responsibility : create new thread and run one diag dump function
 *
 * Parameters
 *                : daemon
 *                : cmd_type - basic  or advanced
 *                : cmd_argc
 *                : vty
 *                : fd - file descriptor
 *
 * Returns        : 0 on success and nonzero on failure
 *
 */
static int
vtysh_diag_dump_create_thread(char* daemon , char **cmd_type ,
        int cmd_argc , struct vty *vty , int fd )
{
    pthread_t tid;
    thread_arg arg;
    struct timespec   ts;
    int error;
    int threadCancelled = TRUE;

    gDiagDumpCommandFailed = FALSE;
    arg.daemon = daemon ;
    arg.cmd_type = cmd_type ;
    arg.cmd_argc = cmd_argc ;
    arg.vty = vty;
    arg.fd = fd ;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += FUN_MAX_TIME;
    if(pthread_create(&tid,NULL,vtysh_diag_dump_thread,(void *)&arg))
    {
        VLOG_ERR("unable to create diag dump command thread");
        return 1;
    }
    dd_mutex_lock(&gDiagDumpwaitMutex);
    error = pthread_cond_timedwait(&gDiagDumpWaitCond, &gDiagDumpwaitMutex,&ts);
    dd_mutex_unlock(&gDiagDumpwaitMutex);

    if(gDiagDumpUserInterruptAlarm == TRUE)
    {
        dd_alarm(0);
        threadCancelled = FALSE ;
        gDiagDumpUserInterruptAlarm = FALSE;
    }

    if(error == ETIMEDOUT)
    {
        error = pthread_cancel(tid);
        if(error != 0)
        {
            VLOG_ERR("unable to cancel diag dump command thread");
            /* we should be able to cancel the thread */
        }

        /*wait for the cleanup function to complete */
        dd_mutex_lock( &gDiagDumpCleanupMutex );
        dd_mutex_unlock( &gDiagDumpCleanupMutex );

        vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
        vty_out(vty,"Deamon %s Timed Out %s",daemon,VTY_NEWLINE);
        VLOG_ERR("Daemon :%s Timed Out - %d secs",
                        daemon,FUN_MAX_TIME);
        vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
        gDiagDumpThreadCancelled = TRUE;
        gDiagDumpCommandFailed = TRUE ;
    }
    else if(gDiagDumpUserInterrupt)
    {
        if(threadCancelled)
        {
            pthread_cancel(tid); /* pthread_cancle can fail here as well */

            /*wait for the cleanup function to complete */
            dd_mutex_lock( &gDiagDumpCleanupMutex );
            dd_mutex_unlock( &gDiagDumpCleanupMutex );

            vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
            vty_out(vty,"Daemon %s terminated due to user interrupt CTRL+ C %s",
                daemon,VTY_NEWLINE);
            vty_out(vty,"%s%s",CLI_STR_HYPHEN,VTY_NEWLINE);
                gDiagDumpThreadCancelled = TRUE;
        }
    }
    /* Need to join the thread so, that thread resources will be freed*/
    if(pthread_join(tid, NULL))
    {
        VLOG_ERR("unable to join with diag dump command thread");
        return 1;
    }
    if(gDiagDumpCommandFailed || (gDiagDumpUserInterrupt && threadCancelled))
    {
        return 1;
    }
    return 0;
}
/* Function       : diagdump_user_interrupt_handler
 * Resposibility  : ctrl c handler for diagDump
 * Return         : NULL
 */
void diagdump_user_interrupt_handler(int signum)
{
    dd_mutex_lock( &gDiagDumpwaitMutex );
    pthread_cond_signal( &gDiagDumpWaitCond );
    dd_mutex_unlock( &gDiagDumpwaitMutex );

    gDiagDumpUserInterruptAlarm = FALSE;
}
/* Function       : diagdump_signal_handler
 * Resposibility  : signal handler for diagdump
 * Return         : NULL
 */
void
diagdump_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
    struct vty * vty = gVty;
    gDiagDumpUserInterrupt = TRUE ;
    /*tigger the alarm only if it is not yet tiggered .*/
    vty_out(vty,"USER INTERRUPT:Diag dump will be terminated in 10 secs%s"
          ,VTY_NEWLINE);
    if(!gDiagDumpUserInterruptAlarm)
    {
        gDiagDumpUserInterruptAlarm = TRUE;
        dd_alarm(USER_INT_ALARM);
    }
}


/*
 * Function       : vtysh_diag_read_pid_file
 * Responsibility : read pid file
 *
 * Parameters
 *                : pidfile
 *
 * Returns        : Negative integer value on failure
 *                  Positive integer value on success
 *
 * Note : read_pidfile() API is does same thing but it opens a file in "r+"
 *        mode. read_pidfile() API will success only if user has "rw"permission.
 *        If user has "r" permission then read_pidfile() API fails.
 */

static int
vtysh_diag_read_pid_file (char *pidfile)
{
    FILE *fp = NULL;
    int pid = 0;
    int rc = 0;
    char err_buf[MAX_CLI_STR_LEN] = {0};

    if(pidfile == NULL) {
        VLOG_ERR("Invalid parameter pidfile");
        return -1;
    }

    fp = fopen(pidfile,"r");
    if (fp == NULL) {
        strerror_r (errno,err_buf,sizeof(err_buf));
        STR_SAFE(err_buf);
        VLOG_ERR("Failed to open pidfile:%s , error:%s",pidfile,err_buf);
        return -1;
    }

    rc = fscanf(fp, "%d", &pid);
    fclose(fp);

    /*
     * valid pid range : 1 to 65536
     * digit count range of valid pid : 1 to 5
     */

    if ((( rc >= MIN_PID_LEN ) && ( rc <= MAX_PID_LEN )) &&
            (( pid >= MIN_PID ) && ( pid <= MAX_PID ))) {
        return pid;
    }
    else {
        VLOG_ERR("Pid value is not in range : (%d-%d), pid : %d",
                MIN_PID , MAX_PID , pid);
        return -1;
    }
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
 * Function       : vtysh_diag_check_crete_dir
 * Responsibility : checks a directory ,
 *                  creates directory if directory is not present
 *
 * Parameters     : void
 * Returns        : 0 on success and nonzero on failure
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

            rc = mkdir (dir, ACCESSPERMS );
            if (  rc == 0) {
                rc = chmod (dir, ACCESSPERMS ) ;
            }
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
                rc = mkdir (dir, ACCESSPERMS);
                if (  rc == 0 ) {
                    rc = chmod (dir, ACCESSPERMS ) ;
                }

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
 * Returns        : 0 on success and nonzero on failure
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
    if ( !initialized ) {
        feature_head = get_feature_mapping();
        if ( feature_head == NULL ) {
            vty_out(vty,"%s%s", ERR_STR ,VTY_NEWLINE);
            return  CMD_WARNING ;
        }
        else {
            initialized = 1;
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
    struct sigaction oldSignalHandler,newSignalHandler,
                oldAlarmHandler,newAlarmHandler;
    int return_val = CMD_SUCCESS;

    /* init global var */
    gDiagDumpUserInterrupt      = FALSE;
    gDiagDumpThreadCancelled    = FALSE;
    gDiagDumpUserInterruptAlarm = FALSE;
    gVty                        = vty;

    /*change the signal handler */
    memset (&oldSignalHandler, '\0', sizeof(oldSignalHandler));
    memset (&newSignalHandler, '\0', sizeof(newSignalHandler));
    memset (&oldAlarmHandler, '\0', sizeof(oldAlarmHandler));
    memset (&newAlarmHandler, '\0', sizeof(newAlarmHandler));

    newSignalHandler.sa_sigaction = diagdump_signal_handler;
    newSignalHandler.sa_flags = SA_SIGINFO;

    newAlarmHandler.sa_handler =  diagdump_user_interrupt_handler ;

    if(sigaction(SIGINT, &newSignalHandler, &oldSignalHandler) != 0)
    {
      VLOG_ERR("Failed to change signal handler");
      vty_out(vty, "diag dump init failed %s" ,VTY_NEWLINE);
      return CMD_WARNING;
    }
    if(sigaction(SIGALRM, &newAlarmHandler, &oldAlarmHandler) != 0)
    {
      VLOG_ERR("Failed to change Alarm handler");
      vty_out(vty, "diag dump init failed %s" ,VTY_NEWLINE);
      if(sigaction(SIGINT, &oldSignalHandler, NULL) != 0)
      {
         VLOG_ERR("Failed to change signal handler to old state");
         exit(0); //should never hit this place
         /* we changed the CLI behavior */
      }
      return CMD_WARNING;
    }


    fun_argv[1] = (char *)  argv[0];
    fun_argv[0] = DIAG_BASIC;

    if ( !initialized ) {
        feature_head  = get_feature_mapping();
        if ( feature_head == NULL ) {
            vty_out(vty,"%s%s", ERR_STR ,VTY_NEWLINE);
            return_val = CMD_WARNING ;
            goto EXIT_FUN;
        }
        else {
            initialized = 1;
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
                return_val = CMD_WARNING ;
                goto EXIT_FUN;
            }

            if ( argv[1][0] == '/') {
                vty_out (vty,"please provide filename without /%s",VTY_NEWLINE);
                return_val = CMD_WARNING ;
                goto EXIT_FUN;
            }

            if ( strlen(argv[1]) > USER_FILE_LEN_MAX ) {
                vty_out (vty,"please provide filename less than %d %s",
                        USER_FILE_LEN_MAX ,VTY_NEWLINE);
                return_val = CMD_WARNING ;
                goto EXIT_FUN;
            }

            snprintf(file_path,sizeof(file_path),"%s/%s",DIAG_DUMP_DIR,argv[1]);
            STR_SAFE(file_path);

            fd = open (file_path, O_CREAT|O_EXCL|O_WRONLY);
            if ( !VALID_FD_CHECK (fd) ) {
                strerror_r (errno,err_buf,sizeof(err_buf));
                STR_SAFE(err_buf);


                VLOG_ERR("failed to open file error:%d,file:%s",
                         errno, file_path);
                vty_out (vty, "failed to open file error:%s file:%s%s",
                        err_buf , file_path,VTY_NEWLINE);
                return_val = CMD_WARNING ;
                goto EXIT_FUN;
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
            if(gDiagDumpUserInterrupt)
            {
               goto USER_INTERRUPT;
            }
            rc = vtysh_diag_dump_create_thread(iter_daemon->name, fun_argv,
                     fun_argc, vty, fd );
            /*Count daemon responded */
            if (!rc) {
                VLOG_DBG("daemon :%s captured diag dump , rc:%d",
                        iter_daemon->name,rc);
                daemon_resp++;
            }
            else {
                VLOG_ERR("daemon :%s failed to capture diag dump , rc:%d",
                        iter_daemon->name,rc);
            }
            if(gDiagDumpUserInterrupt)
            {
               goto USER_INTERRUPT;
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
        return_val = CMD_WARNING ;
        goto EXIT_FUN;
    }

    /* Success rate 100% . All daemons responded */
    if ( daemon_count  ==  daemon_resp ) {
        vty_out(vty,"Diagnostic dump captured for feature %s %s",
                argv[0],VTY_NEWLINE);
    }
    /* Few daemons failed */
    else {
        vty_out(vty,"Diagnostic dump %s feature failed for %d %s %s",
                argv[0], ( daemon_count - daemon_resp ),
                (( daemon_count - daemon_resp ) > 1 ) ? "daemons":"daemon",
                VTY_NEWLINE);
    }

    USER_INTERRUPT:
    if ( VALID_FD_CHECK (fd) && gDiagDumpUserInterrupt )
    {
        /* print ===== */
        FEATURE_END

        snprintf(write_buff,sizeof(write_buff),
                "USER INTERRUPT:Diag dump terminated\n");
        STR_SAFE(write_buff);
        write(fd,write_buff,strlen(write_buff));

        /* print ===== */
        FEATURE_END
    }
    CLOSE(fd);
    if(gDiagDumpUserInterrupt)
    {
      vty_out(vty,"USER INTERRUPT:Diag dump terminated%s"
           ,VTY_NEWLINE);
    }
    EXIT_FUN:
    if(sigaction(SIGINT, &oldSignalHandler, NULL) != 0)
    {
      VLOG_ERR("Failed to change signal handler to old state");
      exit(0); //should never hit this place
      /* we changed the CLI behavior */
    }
    if(sigaction(SIGALRM, &oldAlarmHandler, NULL) != 0)
    {
      VLOG_ERR("Failed to change Alarm handler to old state");
      exit(0); //should never hit this place
      /* we changed the CLI behavior */
    }
    if(gDiagDumpThreadCancelled)
    {
      VLOG_ERR("thread has been cancelled");
    }
    return return_val;

#undef FEATURE_BEGIN
#undef FEATURE_END
}

/*
 * Function       : vtysh_diag_connect_to_target
 * Responsibility : populates jsonrpc client structure for a daemon
 * Parameters     : target  - daemon name
 * Returns        : jsonrpc client on success
 *                  NULL on failure
 *
 */

static struct jsonrpc *
vtysh_diag_connect_to_target(const char *target)
{
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

        pid = vtysh_diag_read_pid_file(pidfile_name);
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
 * Returns        : 0 on success and nonzero on failure
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
    char *cmd_result=NULL, *cmd_error=NULL;
    int rc=0;
    char  diag_cmd_str[DIAG_CMD_LEN_MAX] = {0};

    client = NULL ;
    if  (!(daemon && cmd_type)) {
        VLOG_ERR("invalid parameter daemon or command ");
        return CMD_WARNING;
    }

    if (!vtysh_diag_connect_to_target(daemon)) {
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
        client = NULL;
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
        client = NULL;
        FREE(cmd_result);
        FREE(cmd_error);
        return CMD_WARNING;
    }


    jsonrpc_close(client);
    client = NULL ;
    FREE(cmd_result);
    FREE(cmd_error);
    return CMD_SUCCESS;

#undef  FEATURE_BEGIN
#undef  FEATURE_END
}
