/* System SHOW_EVENTS CLI commands
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
 * File: show_events_vty.c
 *
 * Purpose: To Run Show Events Commands from CLI
 */
#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "show_events_vty.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"
#include "time.h"
#include "systemd/sd-journal.h"
#include <string.h>

VLOG_DEFINE_THIS_MODULE (vtysh_show_events_cli);


/* Function       : get_values
 * Responsibility : read only values from keys
 * return         : return value
 */

const char*
get_value(const char *str)
{
   if(!str) {
      return NULL;
   }
   while(*str!='\0')
   {

      /*found the split*/
      if(*str == '=')  {
          if(*(str+1))  {
             /*value is present*/
               return str+1;
           }
          return NULL;
      }
      str++;
   }
return NULL;
}

/*Function  : convert_to_datetime
 * Responsibility : to convert the real timestamp in to unix timestamp date-time
 * return : none
 */

void
convert_to_datetime(char *buf,int buf_size,const char *str)
{
     int strl = 0;
     char timestring[BUF_SIZE] = {0,};
     char basetime[BASE_SIZE] = {0,};
     char microsec[MICRO_SIZE] = {0,};
     time_t t_t = 0;
     struct tm *tmp = NULL;
     strl = strlen(str);

      /*strl must be greater than or equal to MIN_SIZE (6)*/
     if(strl>=MIN_SIZE) {

        /*copying first 10 characters from str to basetime */
       strncpy(basetime,str,(strl-MIN_SIZE));

        /*copying last 6 characters from str to microsec*/
       strncpy(microsec,str+(strl-MIN_SIZE),MIN_SIZE);

       microsec[MIN_SIZE]=0;
       basetime[strl-MIN_SIZE]=0;

        /*convert basetime string in to integer*/
       t_t = atoi(basetime);
        /*convert in to local time using localtime() API from time.h*/
       tmp = localtime(&t_t);
        /*using strftime() API broken-down time(tmp) according to format specified
              and store the result in to timestring of size(buf_size)*/
       strftime(timestring,BUF_SIZE,"%Y-%m-%d:%H:%M:%S",tmp);

       snprintf(buf,buf_size,"%s.%s",timestring,microsec);
     }
}

/* Function       : cli_show_events
 * Resposibility  : Display Event Logs
 * Return         : 0 on success 1 otherwise
 */
int
cli_show_events(void)
{
  int return_value = 0;
  int events_display_count = 0;
  sd_journal *journal_handle = NULL;

  /* Open Journal File to read Event Logs */
  return_value = sd_journal_open(&journal_handle,SD_JOURNAL_LOCAL_ONLY);

  if(return_value < 0) {
    VLOG_ERR("Failed to open journal");
    return CMD_WARNING;
  }

  /* Filter Event Logs from other Journal Logs */
  return_value = sd_journal_add_match(journal_handle,MESSAGE_OPS_EVT_MATCH,0) ;
  if(return_value < 0) {
    VLOG_ERR("Failed to log the events");
    return CMD_WARNING;
  }

  /* Success, Now print the Header */
  vty_out(vty,"%s---------------------------------------------------%s",
          VTY_NEWLINE,VTY_NEWLINE);
  vty_out(vty,"%s%s","show event logs",VTY_NEWLINE);
  vty_out(vty,"---------------------------------------------------%s",
          VTY_NEWLINE);

  /* For Each Event Log Message  */
  SD_JOURNAL_FOREACH(journal_handle)
  {
    const char *message_data = NULL;
    const char *timestamp = NULL;
    const char *module_name = NULL;
    const char ch = '|';
    char  tm_buf[BUF_SIZE] = {0,};
    const char *tm = NULL;
    const char *msg = NULL;
    const char *message = NULL;
    const char *module = NULL;
    size_t data_length = 0;
    size_t timestamp_length = 0;
    size_t module_length = 0;

    return_value = sd_journal_get_data(journal_handle
                                       , "MESSAGE"
                                       ,(const void **)&message_data
                                       , &data_length);
    if (return_value < 0) {
      VLOG_ERR("Failed to read message field: %s\n", strerror(-return_value));
      continue;
    }

    return_value = sd_journal_get_data(journal_handle
                                       , "SYSLOG_IDENTIFIER"
                                       ,(const void **)&module_name
                                       , &module_length);
    if (return_value < 0) {
      VLOG_ERR("Failed to read module name field: %s\n", strerror(-return_value));
      continue;
    }

    return_value = sd_journal_get_data(journal_handle
                                      ,"_SOURCE_REALTIME_TIMESTAMP"
                                      ,(const void **)&timestamp
                                      , &timestamp_length);
    if (return_value < 0) {
      VLOG_ERR("Failed to read timestamp field: %s\n", strerror(-return_value));
      continue;
    }

    ++events_display_count;

    /*to get the values from fields using get_value() API*/

    msg = get_value(message_data);

    if(msg!=NULL) {
         message = strchr(msg,ch);
    }
    else {
        VLOG_ERR("failed to read message-value from message field");
        message =NULL;
    }

    module = get_value(module_name);

    if(module==NULL) {
      VLOG_ERR("failed to read module-value from module field");
    }

    tm = get_value(timestamp);

    if(tm!=NULL) {
      /*convert real timestamp to unix timestamp */
       convert_to_datetime(tm_buf,BUF_SIZE,tm);
    }
    else {
       VLOG_ERR("failed to read time-value from time field");
    }

    vty_out(vty,"%s|%s%s%s",tm_buf,module,message,VTY_NEWLINE);
  }

  if(!events_display_count) {
    vty_out(vty,"No event has been logged in the system%s",VTY_NEWLINE);
  }
  return CMD_SUCCESS;
}


/*
* Action routine for show events
*/
DEFUN_NOLOCK (cli_platform_show_events,
  cli_platform_show_events_cmd,
  "show events",
  SHOW_STR
  SHOW_EVENTS_STR)
  {
    return cli_show_events();
  }
