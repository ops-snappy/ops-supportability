/* System SUPPORTABILITY CLI commands
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
* File: supportability_vty.c
*
* Purpose: To Install all Supportability CLI Commands
*/

#include "vtysh/command.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "vtysh/memory.h"
#include "dynamic-string.h"
#include "supportability_vty.h"

/*
 * Function           : cli_pre_init
 * Responsibility     : Install the cli nodes
 */

void
cli_pre_init(void)
{
   /* Supportability Doesnt have any new node */

}

/*
 * Function           : cli_post_init
 * Responsibility     : Install the cli action routines
 *                      This function is common across all supportability cli
 *                      Install all supportability cli here
 */
void
cli_post_init()
{
  install_element (ENABLE_NODE, &cli_platform_show_tech_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_tech_list_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_tech_feature_cmd);
  install_element (ENABLE_NODE, &cli_platform_show_events_cmd);

}
