
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
#include <yaml.h>
#define TRUE                  1
#define MAX_SEVS              8
#define SEVERITY_LEVEL_DBG    "Display logs with all severities\n"
#define SEVERITY_LEVEL_INFO   "Display logs with severity 'info(1)' and above\n"
#define SEVERITY_LEVEL_NOTICE "Display logs with severity 'notice(2)' and above\n"
#define SEVERITY_LEVEL_WARN   "Display logs with severity 'warning(3)' and above\n"
#define SEVERITY_LEVEL_ERR    "Display logs with severity 'error(4)' and above\n"
#define SEVERITY_LEVEL_CRIT   "Display logs with severity 'critical(5)' and above\n"
#define SEVERITY_LEVEL_ALERT  "Display logs with severity 'alert(6)' and above\n"
#define SEVERITY_LEVEL_EMER   "Display logs with severity 'emergency(7)' only\n"
#define MAX_FEATURES          100
#define MAX_FEATURE_NAME_SIZE 50
#define MAX_CMD_SIZE          (MAX_FEATURES*MAX_FEATURE_NAME_SIZE)
#define MAX_HELP_SIZE         256
#define MAX_EV_CATEGORIES     999
#define MAX_FEATURE_HELP_SIZE (MAX_FEATURES*MAX_HELP_SIZE)
#define MAX_EV_HELP_SIZE      (MAX_EV_CATEGORIES*MAX_HELP_SIZE)
#define PID_DIRECTORY         "/run/openvswitch/"
#define MAX_FILENAME_SIZE     100
#define MAX_DAEMONS           200
#define MAX_VLOG_CMD          (MAX_DAEMONS*MAX_FEATURE_NAME_SIZE)

extern char * get_yaml_tokens(yaml_parser_t *parser,  yaml_event_t **tok, FILE *fh);
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
extern struct cmd_element cli_platform_show_vlog_cmd;
extern struct cmd_element cli_platform_show_vlog_config_list_cmd;
extern struct cmd_element cli_platform_show_vlog_feature_cmd;
extern struct cmd_element cli_config_vlog_set_cmd;
/* Syslog Command */

extern struct cmd_element vtysh_config_syslog_basic_cmd;
extern struct cmd_element vtysh_config_syslog_udp_cmd;
extern struct cmd_element vtysh_config_syslog_tcp_cmd;
extern struct cmd_element vtysh_config_syslog_svrt_cmd;
extern struct cmd_element vtysh_config_syslog_udp_svrt_cmd;
extern struct cmd_element vtysh_config_syslog_tcp_svrt_cmd;
extern struct cmd_element vtysh_config_syslog_prot_svrt_noport_cmd;


extern struct cmd_element no_vtysh_config_syslog_basic_cmd;
extern struct cmd_element no_vtysh_config_syslog_udp_cmd;
extern struct cmd_element no_vtysh_config_syslog_tcp_cmd;
extern struct cmd_element no_vtysh_config_syslog_svrt_cmd;
extern struct cmd_element no_vtysh_config_syslog_udp_svrt_cmd;
extern struct cmd_element no_vtysh_config_syslog_tcp_svrt_cmd;
extern struct cmd_element no_vtysh_config_syslog_prot_svrt_noport_cmd;
extern struct cmd_element vtysh_config_no_syslog_cmd;

extern void syslog_ovsdb_init(void);
extern struct cmd_element cli_platform_copy_core_dump_tftp_cmd;
extern struct cmd_element cli_platform_copy_core_dump_sftp_cmd;
extern struct cmd_element cli_platform_copy_core_dump_kernel_tftp_cmd;
extern struct cmd_element cli_platform_copy_core_dump_kernel_sftp_cmd;
extern struct cmd_element cli_platform_copy_core_dump_tftp_cmd_inst;
extern struct cmd_element cli_platform_copy_core_dump_sftp_cmd_inst;
#endif /* _SUPPORTABILITY_VTY_H_ */
