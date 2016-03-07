
/* Supportability Command Declaration file
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
 * File: supportability_vty.h
 *
 * Purpose: header file for supportability command structs
 */

#ifndef _SUPPORTABILITY_VTY_H_

#define _SUPPORTABILITY_VTY_H_

#define MAX_SEVS  8
#define SEVERITY_LEVEL_EMER   "Display logs with LOG_EMER severity\n"
#define SEVERITY_LEVEL_ALERT  "Display logs with LOG_ALERT severity\n"
#define SEVERITY_LEVEL_CRIT   "Display logs with LOG_CRIT severity\n"
#define SEVERITY_LEVEL_ERR    "Display logs with LOG_ERR severity\n"
#define SEVERITY_LEVEL_WARN   "Display logs with LOG_WARN severity\n"
#define SEVERITY_LEVEL_NOTICE "Display logs with LOG_NOTICE severity\n"
#define SEVERITY_LEVEL_INFO   "Display logs with LOG_INFO severity\n"
#define SEVERITY_LEVEL_DBG    "Display logs with LOG_DEBUG severity\n"

extern struct cmd_element vtysh_diag_dump_list_cmd;
extern struct cmd_element vtysh_diag_dump_cmd;
extern struct cmd_element cli_platform_show_tech_cmd;
extern struct cmd_element cli_platform_show_tech_list_cmd;
extern struct cmd_element cli_platform_show_tech_feature_cmd;
extern struct cmd_element cli_platform_show_tech_file_cmd;
extern struct cmd_element cli_platform_show_tech_file_force_cmd;
extern struct cmd_element cli_platform_show_tech_feature_file_cmd;
extern struct cmd_element cli_platform_show_tech_feature_file_force_cmd;
extern struct cmd_element cli_platform_show_events_cmd;
extern struct cmd_element cli_platform_show_core_dump_cmd;
extern struct cmd_element cli_platform_show_vlog_config_cmd;
extern struct cmd_element cli_platform_show_vlog_config_list_cmd;
extern struct cmd_element cli_platform_show_vlog_feature_cmd;
extern struct cmd_element cli_config_vlog_set_cmd;
#endif /* _SUPPORTABILITY_VTY_H_ */
