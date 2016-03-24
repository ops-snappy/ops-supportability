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
#include <pthread.h>
#include <signal.h>

VLOG_DEFINE_THIS_MODULE (vtysh_show_tech_cli);

#define     READ_LINE_SIZE    256
#define     BASH_CAT_CMD      "bashcat"
#define     CMD_MAX_TIME      60       //in secs
#define     USER_INT_ALARM    10       //in secs

#ifndef FALSE
#define     FALSE             0
#endif

#ifndef TRUE
#define     TRUE              1
#endif

void user_interrupt_handler(int );

/* show tech global var should be intialized to zero */
bool gUserInterrupt        = FALSE ;
bool gUserInterruptAlarm   = FALSE ;
bool gCommandFailed        = FALSE ;
bool gThreadCancelled      = FALSE ;
int  gVtyOldType           = 0     ;
int  gUserInterruptVtyType = -1;

pthread_mutex_t waitMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cleanupMutex = PTHREAD_MUTEX_INITIALIZER;


extern pthread_mutex_t vtysh_ovsdb_mutex;

/* Function       : st_mutex_lock
 * Resposibility  : wrapper for mutex lock sys call, to log failed state
 * Return         : NULL
 */
int st_mutex_lock(pthread_mutex_t *mutex)
{
    int rc = -1 ;
    rc = pthread_mutex_lock(mutex);
    if(rc)
    {
        VLOG_ERR("show tech:mutex lock failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : st_mutex_unlock
 * Resposibility  : wrapper for mutex unlock sys call, to log failed state
 * Return         : NULL
 */
int st_mutex_unlock(pthread_mutex_t *mutex)
{
    int rc = -1 ;
    rc = pthread_mutex_unlock(mutex);
    if(rc)
    {
        VLOG_ERR("show tech:mutex unlock failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : st_alarm
 * Resposibility  : wrapper for alarm sys call, to log failed state
 * Return         : NULL
 */
unsigned st_alarm(unsigned seconds)
{
    int rc = -1 ;
    rc = alarm(seconds);
    if(rc)
    {
        VLOG_ERR("show tech:alarm failed, rc =%d", rc);
    }
    return rc;
}

/* Function       : exec_showtech_cmd_on_thread
 * Resposibility  : Parse the cli command and execute as needed on new thread
 * Return         : CMD_SUCCESS on success CMD_WARNING otherwise
 */
int
exec_showtech_cmd_on_thread(const char* cmd)
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
/* Function       : cmd_thread_cleanup_handler
 * Resposibility  : to clean the cmd thread , currently it will release
                    vtysh_ovsdb_mutex
 * Return         : NULL
 */
void cmd_thread_cleanup_handler(void *arg )
{
   //VTYSH_OVSDB_UNLOCK;
   pthread_mutex_trylock(&vtysh_ovsdb_mutex); /* locks the mutex if thread don't
                                                 own it */
   pthread_mutex_unlock(&vtysh_ovsdb_mutex);
   pthread_mutex_unlock( &cleanupMutex );
}

/* Function       : showtech_cmd_thread
 * Resposibility  : call the function which parse and execute the command
 *                  set the global var respectively
 * Return         : NULL , it sets the global var gCommadFail to CMD_SUCCESS on
 *                  success CMD_WARNING otherwise
 */
void *
showtech_cmd_thread(void * cmd)
{
  if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
   {
       VLOG_ERR("show tech - unable to set cancel state of new thread");
       return (void *) 0;
   }
   if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL))
   {
       VLOG_ERR("show tech - unable to set cancel type of new thread");
       return (void *) 0;
   }
   pthread_cleanup_push(cmd_thread_cleanup_handler, NULL);

   st_mutex_lock( &cleanupMutex );

   if(exec_showtech_cmd_on_thread((char *)cmd) != CMD_SUCCESS )
       gCommandFailed = TRUE ;

   st_mutex_lock( &waitMutex );
   if(pthread_cond_signal( &waitCond ))
   {
       VLOG_ERR("show tech - unable to enable signal main thread");
   }
   st_mutex_unlock( &waitMutex );
   pthread_cleanup_pop(0);
   st_mutex_unlock( &cleanupMutex );
   return (void *) 0;
}
/* Function       : exec_showtech_cmd
 * Resposibility  : Parse the cli command and execute as needed
 *                  it is just a wrapper , it created a new thread and call the
 *                  function which parse and execute the command
 * Return         : CMD_SUCCESS on success CMD_WARNING otherwise
 */
int
exec_showtech_cmd(const char* cmd)
{
    pthread_t tid;
    struct timespec   ts;
    int error;
    int threadCancelled = TRUE;

    gCommandFailed = FALSE;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += CMD_MAX_TIME;
    if(pthread_create(&tid,NULL,showtech_cmd_thread,(void *)cmd))
    {
        VLOG_ERR("unable to create show tech command thread");
        return 1;
    }
    st_mutex_lock(&waitMutex);
    error = pthread_cond_timedwait(&waitCond, &waitMutex,&ts);
    st_mutex_unlock(&waitMutex);

    if(gUserInterruptAlarm == TRUE)
    {
        st_alarm(0);
        threadCancelled = FALSE ;
        gUserInterruptAlarm = FALSE;
    }

    if(error == ETIMEDOUT)
    {
        error = pthread_cancel(tid);
        if(error != 0)
        {
            VLOG_ERR("unable to cancel show tech command thread");
            // we should be able to cancel the thread
        }

        /*wait for the cleanup function to complete */
        st_mutex_lock( &cleanupMutex );
        st_mutex_unlock( &cleanupMutex );

        vty_out(vty,"%s---------------------------------%s"
                ,VTY_NEWLINE,VTY_NEWLINE);
        vty_out(vty,"Command %s Timed Out%s",(char *)cmd,VTY_NEWLINE);
        vty_out(vty,"---------------------------------%s"
            ,VTY_NEWLINE);
        gThreadCancelled = TRUE;
        gCommandFailed = TRUE ;
    }
    else if(gUserInterrupt)
    {
        if(threadCancelled)
        {
            pthread_cancel(tid);

            /*wait for the cleanup function to complete */
            st_mutex_lock( &cleanupMutex );
            st_mutex_unlock( &cleanupMutex );

        }

        if(gUserInterruptVtyType != -1)
        {
            vty->type = gUserInterruptVtyType ;
        }
        if(gVtyOldType != VTY_FILE && gUserInterruptVtyType != VTY_FILE)
        {
            buffer_reset(vty->obuf);
        }

        if(threadCancelled)
        {
            vty_out(vty,"%s---------------------------------%s"
                ,VTY_NEWLINE,VTY_NEWLINE);
            vty_out(vty,"Command %s terminated due to user interrupt CTRL+ C %s",
                   (char *)cmd,VTY_NEWLINE);
            vty_out(vty,"---------------------------------%s"
                ,VTY_NEWLINE);
            gThreadCancelled = TRUE;
        }
    }
    /* Need to join the thread so, that thread resources will be freed*/
    if(pthread_join(tid, NULL))
    {
        VLOG_ERR("unable to join with show tech command thread");
        return 1;
    }
    if(gCommandFailed || (gUserInterrupt && threadCancelled))
    {
        return CMD_WARNING;
    }
    return CMD_SUCCESS;
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
/* Function       : user_interrupt_handler
 * Resposibility  : ctrl c handler for showtech
 * Return         : NULL
 */
void user_interrupt_handler(int signum)
{
    st_mutex_lock( &waitMutex );
    pthread_cond_signal( &waitCond );
    st_mutex_unlock( &waitMutex );

    gUserInterruptAlarm = FALSE;
}
/* Function       : showtech_signal_handler
 * Resposibility  : signal handler for showtech
 * Return         : NULL
 */
void
showtech_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
    gUserInterrupt = TRUE ;
    //tigger the alarm only if it is not yet tiggered .
    if(!gUserInterruptAlarm)
    {
        gUserInterruptAlarm = TRUE;
        vty_out(vty,"USER INTERRUPT:Show tech will be terminated with in 10 sec %s"
               ,VTY_NEWLINE);
        gUserInterruptVtyType = vty->type ;
        vty->type = VTY_FILE;
        st_alarm(USER_INT_ALARM);
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
   struct sigaction oldSignalHandler,newSignalHandler,
                oldAlarmHandler,newAlarmHandler;
   int return_val = CMD_SUCCESS;

   /* init global var */
   gUserInterrupt      = FALSE;
   gThreadCancelled    = FALSE;
   gUserInterruptAlarm = FALSE;
   gUserInterruptVtyType = -1;

   /*change the signal handler */
   memset (&oldSignalHandler, '\0', sizeof(oldSignalHandler));
   memset (&newSignalHandler, '\0', sizeof(newSignalHandler));
   memset (&oldAlarmHandler, '\0', sizeof(oldAlarmHandler));
   memset (&newAlarmHandler, '\0', sizeof(newAlarmHandler));

   newSignalHandler.sa_sigaction = showtech_signal_handler;
   newSignalHandler.sa_flags = SA_SIGINFO;

   newAlarmHandler.sa_handler =  user_interrupt_handler ;

   if(sigaction(SIGINT, &newSignalHandler, &oldSignalHandler) != 0)
   {
      VLOG_ERR("Failed to change signal handler");
      vty_out(vty, "show tech init failed %s" ,VTY_NEWLINE);
      return CMD_WARNING;
   }
   if(sigaction(SIGALRM, &newAlarmHandler, &oldAlarmHandler) != 0)
   {
      VLOG_ERR("Failed to change Alarm handler");
      vty_out(vty, "show tech init failed %s" ,VTY_NEWLINE);
      if(sigaction(SIGINT, &oldSignalHandler, NULL) != 0)
      {
         VLOG_ERR("Failed to change signal handler to old state");
         exit(0); //should never hit this place
         /* we changed the CLI behavior */
      }
      return CMD_WARNING;
   }

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
      return_val = CMD_WARNING;
      goto EXIT_FUN;
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
               if(gUserInterrupt)
               {
                  iter_cli->command_failed = 0;
                  goto USER_INTERRUPT;
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
                  if(gUserInterrupt)
                  {
                     goto USER_INTERRUPT;
                  }
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
                  if(gUserInterrupt)
                  {
                     iter_cli->command_failed = 0;
                     goto USER_INTERRUPT;
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
                  if(gUserInterrupt)
                  {
                     goto USER_INTERRUPT;
                  }
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
                  if(gUserInterrupt)
                  {
                     iter_cli->command_failed = 0;
                     goto USER_INTERRUPT;
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
         return_val = CMD_SUCCESS;
         goto EXIT_FUN;
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
   USER_INTERRUPT:
   if(gUserInterrupt)
   {
      vty_out(vty,"USER INTERRUPT:Show tech terminated%s"
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
   if(gThreadCancelled)
   {
      VLOG_ERR("thread has been cancelled");
   }
   return return_val;
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
    char* outputbuf;
    gVtyOldType = 0;

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

    gVtyOldType = vty->type;
    vty->type = VTY_FILE;

    cli_show_tech(feature,NULL);

    vty->type = gVtyOldType;
    if(vty->obuf == NULL)
    {
      vty_out(vty,"Show Tech execution failed%s",VTY_NEWLINE);
      close(fd);
      return CMD_WARNING;
    }
    outputbuf = buffer_getstr(vty->obuf);

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
