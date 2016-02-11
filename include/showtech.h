/*
 *  (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License. You may obtain
 *  a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 */

/************************************************************************//**
 * @ingroup ops-supportability
 *
 * @file
 * Header file for show tech configuration parser functions.
 ***************************************************************************/

#ifndef _SHOWTECH_H_
#define _SHOWTECH_H_

#include "supportability_utils.h"

 struct clicmds
 {
  char* command;
  int command_failed;
  struct clicmds* next;
};

struct ovscolm
{
  char* name;
  struct ovscolm* next;
};

struct ovstable
{
  char* tablename;
  struct ovscolm* p_colmname;
  struct ovstable* next;
};

struct sub_feature
{
  char* name;
  char* desc;
  char no_sta_support;
  char support_stb;
  char is_dummy;
  struct clicmds* p_clicmds;
  struct ovstable* p_ovstable;
  struct sub_feature* next;
};

struct feature
{
  char* name;
  char* desc;
  struct sub_feature* p_subfeature;
  struct feature* next;
};

/* Return Show Tech Configuration Datastructure Header */
struct feature* get_showtech_config(const char* config_file);
/* Frees the Show Tech Configuration Datastructure */
void free_show_tech_config(void);

#endif /* _SHOWTECH_H_ */
