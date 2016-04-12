/* SHOW_EVENTS CLI commands.
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * File: show_events_vty.h
 *
 * Purpose: To Run Show Events command from CLI.
 */

#ifndef _SHOW_EVENTS_VTY_H
#define _SHOW_EVENTS_VTY_H

#define SHOW_EVENTS_CMD              "show events {event-id <A:1001-999999>| severity \
                                     (emer | alert | crit | err | warn | notice | info | debug) \
                                     | reverse | category ("
#define SHOW_EVENTS_STR              "Display all log events\n"
#define SHOW_EVENTS_FILTER_EV_ID     "Display log events for specified event IDs\n"
#define SHOW_EVENTS_EV_ID            "Specify the event IDs to display\n"
#define SHOW_EVENTS_SEVERITY         "Display log events as per specified severity\n"
#define SHOW_EVENTS_CATEGORY         "Display log events for specified event category\n"
#define SHOW_EVENTS_FILTER_CAT       "Specify the event category to display\n"
#define SHOW_EVENTS_REVERSE          "Display log events in reverse order (most recent first)\n"
#define MESSAGE_OPS_EVT_MATCH        "MESSAGE_ID=50c0fa81c2a545ec982a54293f1b1945"
#define MAX_FILTER_ARGS              3
#define EVENT_ID_INDEX               0
#define EVENT_SEVERITY_INDEX         1
#define EVENT_CATEGORY_INDEX         3

#define EVENTS_YAML_FILE             "/etc/openswitch/supportability/ops_events.yaml"
#define BUF_SIZE                     100 /*maximum buffer size*/
#define BASE_SIZE                    20  /*maximum base time string size*/
#define MICRO_SIZE                   7   /*maximum micro seconds size*/
#define MIN_SIZE                     6   /*string length must be greater than or equal to MIN_SIZE(6)*/


#endif //_SHOW_EVENTS_VTY_H
