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

#define SHOW_EVENTS_CMD  "show events {event-id <1001-999999>| severity \
                  (emer | alert | crit | err | warn | notice | info | debug) \
                  | reverse | category ("
#define SHOW_EVENTS_STR              "Display event logs\n"
#define SHOW_EVENTS_FILTER_EV_ID     "Filter based on event ID\n"
#define SHOW_EVENTS_EV_ID            "Event ID to filter\n"
#define SHOW_EVENTS_SEVERITY         "Filter based on severity\n"
#define SHOW_EVENTS_CATEGORY         "Filter based on category\n"
#define SHOW_EVENTS_FILTER_CAT       "Event category to filter\n"
#define SHOW_EVENTS_REVERSE          "Reverse list the event logs\n"
#define MESSAGE_OPS_EVT_MATCH "MESSAGE_ID=50c0fa81c2a545ec982a54293f1b1945"
#define MAX_FILTER_ARGS 3
#define EVENT_ID_INDEX 0
#define EVENT_SEVERITY_INDEX 1
#define EVENT_CATEGORY_INDEX 3

#define EVENTS_YAML_FILE "/etc/openswitch/supportability/ops_events.yaml"
#define BUF_SIZE 100
#define BASE_SIZE 20
#define MICRO_SIZE 7
#define MIN_SIZE 6
#define TRUE 1


#endif //_SHOW_EVENTS_VTY_H
