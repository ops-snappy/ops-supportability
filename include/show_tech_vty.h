/* SHOW_TECH CLI commands.
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
 * File: show_tech_vty.h
 *
 * Purpose: To Run Show Tech command from CLI.
 */

#ifndef _SHOW_TECH_VTY_H
#define _SHOW_TECH_VTY_H

void cli_pre_init(void);
void cli_post_init(void);

#define SHOW_TECH_STR              "Display output of a predefined command sequence used by technical support\n"
#define SHOW_TECH_LIST_STR         "Display supported feature groups\n"
#define SHOW_TECH_FILE_STR         "Capture command-output into a specified file\n"
#define SHOW_TECH_FILENAME_STR     "Specify the filename to capture command-output\n"
#define SHOW_TECH_FEATURE_STR      "Display output of feature-specific predefined command sequence used by technical support\n"
#define SHOW_TECH_SUB_FEATURE_STR  "Display output of sub feature-specific predefined command sequence used by technical support\n"
#define SHOW_TECH_FILE_FORCE_STR   "Overwrite if the given file exists\n"
void show_tech_vty_init();

#endif //_SHOW_TECH_VTY_H
