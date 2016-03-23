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
#include "supportability_vty.h"
#include "supportability_utils.h"

VLOG_DEFINE_THIS_MODULE (vtysh_show_events_cli);

/* Function  : convert_to_datetime
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

/* Function       : journal_filter
 * Resposibility  : Filter logs based on journal fields
 * Return         : 0 on success -1 otherwise
 */
int
journal_filter(const char *arg, int index, sd_journal *journal_handle)
{
    char buf[BUF_SIZE];
    int id = 0, return_value = 0, level = 0;
    if(index == EVENT_ID_INDEX) {
        id = atoi(arg);
        snprintf(buf, BUF_SIZE, "OPS_EVENT_ID=%d", id);
    }
    else if(index == EVENT_SEVERITY_INDEX) {
        level = sev_level((char*)arg);
        if((level >= 0) && (level < MAX_SEVS)) {
            snprintf(buf, BUF_SIZE, "PRIORITY=%d", level);
        }
        else {
            return -1;
        }
    }
    else if(index == EVENT_CATEGORY_INDEX) {
        strnupr((char*)arg, strlen(arg));
        snprintf(buf, BUF_SIZE, "OPS_EVENT_CATEGORY=%s", arg);
    }
    else {
        VLOG_ERR("Invalid index value");
        return -1;
    }
    return_value = sd_journal_add_match(journal_handle,buf,0);
    if(return_value < 0) {
      VLOG_ERR("Failed to log the events");
      return -1;
  }
  return 0;
}

/* Function       : cli_show_events
 * Resposibility  : Display Event Logs
 * Return         : 0 on success 1 otherwise
 */
int
cli_show_events(sd_journal *journal_handle,int reverse, int filter)
{
  int return_value = 0;
  int events_display_count = 0;
  int eof = 1;
  /* Success, Now print the Header */
  vty_out(vty,"%s---------------------------------------------------%s",
          VTY_NEWLINE,VTY_NEWLINE);
  vty_out(vty,"%s%s","show event logs",VTY_NEWLINE);
  vty_out(vty,"---------------------------------------------------%s",
          VTY_NEWLINE);

  if(reverse) {
      eof = sd_journal_previous(journal_handle);
      if(eof < 0) {
          VLOG_ERR("sd_journal_previous failed");
          return CMD_WARNING;
      }
  }
  else {
      eof = sd_journal_next(journal_handle);
      if(eof < 0) {
          VLOG_ERR("sd_journal_next failed");
          return CMD_WARNING;
      }
  }
  /* For Each Event Log Message  */
  while(eof)
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
      if(reverse) {
          eof = sd_journal_previous(journal_handle);
          if(eof < 0) {
              VLOG_ERR("sd_journal_previous failed");
              return CMD_WARNING;
          }
      }
      else {
          eof = sd_journal_next(journal_handle);
          if(eof < 0) {
              VLOG_ERR("sd_journal_next failed");
              return CMD_WARNING;
          }
      }
  }

  if(!events_display_count) {
      if(filter) {
          vty_out(vty,"No event match the filter provided%s",VTY_NEWLINE);
      }
      else {
          vty_out(vty,"No event has been logged in the system%s",VTY_NEWLINE);
      }
  }
  sd_journal_close(journal_handle);
  return CMD_SUCCESS;
}


/*
 * Action routine for show events
 */
DEFUN_NOLOCK (cli_platform_show_events,
        cli_platform_show_events_cmd,
        "show events "
        "{event-id <1001-999999>| severity (emer | alert | crit | err | warn | notice | info | debug) | category WORD|reverse}",
        SHOW_STR
        SHOW_EVENTS_STR
        SHOW_EVENTS_FILTER_EV_ID
        SHOW_EVENTS_EV_ID
        SHOW_EVENTS_SEVERITY
        SEVERITY_LEVEL_EMER
        SEVERITY_LEVEL_ALERT
        SEVERITY_LEVEL_CRIT
        SEVERITY_LEVEL_ERR
        SEVERITY_LEVEL_WARN
        SEVERITY_LEVEL_NOTICE
        SEVERITY_LEVEL_INFO
        SEVERITY_LEVEL_DBG
        SHOW_EVENTS_REVERSE
        SHOW_EVENTS_CATEGORY)
{
    int i = 0, return_value = 0, reverse = 0, filter = 0;
    sd_journal *journal_handle = NULL;

    /* Open Journal File to read Event Logs */
    return_value = sd_journal_open(&journal_handle, SD_JOURNAL_LOCAL_ONLY);

    if(return_value < 0) {
        vty_out(vty,"Not able to read the log file%s",VTY_NEWLINE);
        VLOG_ERR("Failed to open journal");
        return CMD_WARNING;
    }
    /* Lets find OPS Event Logs out of journal */
    return_value = sd_journal_add_match(journal_handle, MESSAGE_OPS_EVT_MATCH, 0);
    if(return_value < 0) {
        vty_out(vty,"Not able to read OPS events%s",VTY_NEWLINE);
        VLOG_ERR("Failed to log the events");
        sd_journal_close(journal_handle);
        return CMD_WARNING;
    }

    if(argv[2] != NULL) {
        /* Reverse list option */
        return_value = sd_journal_seek_tail(journal_handle);
        if(return_value < 0) {
            vty_out(vty,"Unable to reverse the logs%s",VTY_NEWLINE);
            VLOG_ERR("sd_journal_seek_tail failed with err %d",
            return_value);
            sd_journal_close(journal_handle);
            return CMD_WARNING;
        }
        reverse = TRUE;
    }
    /* Filter Event Logs based on given filters in CLI */
    while(i <= MAX_FILTER_ARGS)
    {
        if(argv[i] != NULL) {
            if(i != 2) {
                return_value = journal_filter(argv[i], i, journal_handle);
            }
            if(return_value < 0) {
                sd_journal_close(journal_handle);
                vty_out(vty,"Log Filter failed%s",VTY_NEWLINE);
                VLOG_ERR("journal_filter failed");
                return CMD_WARNING;
            }
            filter = TRUE;
        }
        i++;
    }
    return cli_show_events(journal_handle, reverse, filter);
}
