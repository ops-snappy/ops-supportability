/*
 *  (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *  Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Nicira, Inc.
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
 * Source file for the show tech configuration parser
 ***************************************************************************/

#include <stdio.h>
#include <yaml.h>
#include "showtech.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(showtech);

enum
{
  FEATURE,
  FEATURE_NAME,
  FEATURE_DESC,
  SUB_FEATURE,
  SUB_FEATURE_NAME,
  SUB_FEATURE_DESC,
  SUPPORT_SHOWTECH_ALL,
  SUPPORT_SHOWTECH_FEATURE,
  CLI_CMDS,
  OVSDB,
  TABLE,
  TABLE_NAME,
  COLNAMES,
  VALUE,
  MAX_NUM_KEYS
};

static struct feature* feature_head;
static const char* keystr[MAX_NUM_KEYS] =
{
   "feature",
   "feature_name",
   "feature_desc",
   "sub_feature",
   "sub_feature_name",
   "sub_feature_desc",
   "support_showtech_all",
   "support_showtech_feature",
   "cli_cmds",
   "ovsdb",
   "table",
   "table_name",
   "col_names",
   "values"
};




/* Function        : strcmp_with_nullcheck
 * Responsibility  : Ensure arguments are not null before calling strcmp_with_nullcheck
 * Return          : 1 if arguments are null otherwise return value form strcmp_with_nullcheck
 */
int
strcmp_with_nullcheck( const char * str1, const char * str2 )
{
  if(str1 == NULL || str2 == NULL)
    return -1;
  return strcmp(str1,str2);
}



/* Function        : strdup_with_nullcheck
 * Responsibility  : Ensure arguments are not null before calling strdup
 * Return          : null if argument is null otherwise return value form strdump
 */
char *
strdup_with_nullcheck( const char * str1)
{
  if(str1 == NULL)
    return NULL;
  return strdup(str1);
}



/* getValueType
 *
 * Returns the Type of the given string
 *
 */
static int
getValueType (const char *data)
{
  int i = FEATURE;
  for (i = FEATURE; i < VALUE; i++)
  {
    if (!strcmp_with_nullcheck (keystr[i], data))
    {
      return i;
    }
  }
  /* Data didn't match any of the keywords, hence it should be value */
  return VALUE;
}

/* add_feature
 *
 * Creates a new Feature Element and adds it after the
 * afternode element if it is a valid node
 *
 * Return address of the newly created node on success
 * or NULL on failure
 */

static struct feature*
add_feature (struct feature* afternode)
{
  struct feature* elem = NULL;
  elem = (struct feature*) calloc (1, sizeof(struct feature));
  if (elem == NULL)
  {
    VLOG_ERR ("Memory allocation failure\n");
    return NULL;
  }
  /* Is the Node valid */
  if (afternode != NULL)
  {
    elem->next = afternode->next;
    afternode->next = elem;
  }
  return elem;
}

/* add_feature_name
 *
 * Adds Feature name to the given feature element
 *
 * returns 0 on success and -1 on failure
 */

static int
add_feature_name (struct feature* element, const char* feature_name)
{
  if (element == NULL)
  {
    return -1;
  }
  if(element->name != NULL)
  {
    free(element->name);
    element->name = NULL;
  }

  element->name = strdup_with_nullcheck (feature_name);
  return 0;
}

/* add_feature_desc
 *
 * Adds Feature description to the given feature element
 *
 * returns 0 on success and -1 on failure
 *
 */

static int
add_feature_desc (struct feature* element, const char* feature_desc)
{
  if (element == NULL)
  {
    return -1;
  }
  if(element->desc != NULL)
  {
    free(element->desc);
    element->desc = NULL;
  }
  element->desc = strdup_with_nullcheck (feature_desc);
  return 0;
}

/* add_subfeature
 *
 * Adds new Sub Feature Element after the node given
 *
 * return pointer to the node on sucess and NULL on Error
 */

static struct sub_feature*
add_subfeature (struct sub_feature* afternode)
{
  struct sub_feature* elem = NULL;
  elem = (struct sub_feature*) calloc (1, sizeof(struct sub_feature));
  if (elem == NULL)
  {
    VLOG_ERR ("Memory allocation failure\n");
    return NULL;
  }

  if (afternode != NULL)
  {
    elem->next = afternode->next;
    afternode->next = elem;
  }
  return elem;
}

/* add_subfeature_name
 *
 * Adds Sub Feature name to the given sub feature element
 *
 * returns 0 on success and -1 on failure
 */


static int
add_subfeature_name (struct sub_feature* element, const char* sub_feature_name)
{
  if (element == NULL)
  {
    return -1;
  }
  if(element->name != NULL)
  {
    free(element->name);
    element->name = NULL;
  }
  element->name = strdup_with_nullcheck (sub_feature_name);
  return 0;
}

/* add_subfeature_desc
 *
 * Adds Sub Feature description to the given sub feature element
 *
 * returns 0 on success and -1 on failure
 *
 */
static int
add_subfeature_desc (struct sub_feature* element, const char* sub_feature_desc)
{
  if (element == NULL)
  {
    return -1;
  }
  if(element->desc != NULL)
  {
    free(element->desc);
    element->desc = NULL;
  }
  element->desc = strdup_with_nullcheck (sub_feature_desc);
  return 0;
}

/* set_subfeature_as_dummy
 *
 * Sets the Current SubFeature as a Dummy Node.
 *
 * returns 0 on success and -1 on failure
 *
 */
static int
set_subfeature_as_dummy(struct sub_feature* element)
{
   if(element == NULL)
   {
      return -1;
   }
   element->is_dummy = 1;
   return 0;
}

/* add_subfeature_showtechall
 *
 * Show Tech All Flag will be used( in future ) to determine whether to
 * execute the subfeature as part of show tech all command or not.  This
 * function sets this flag according to the configuration
 *
 * returns 0 on success and -1 on failure
 */

static int
add_subfeature_showtechall (struct sub_feature* element,
  const char* showtechall)
{
  if (element == NULL)
  {
    return -1;
  }
  if ((!strcmp_with_nullcheck ("yes", showtechall)) || (!strcmp_with_nullcheck ("true", showtechall)))
  {
    element->no_sta_support = 0;
  }
  else
  {
    element->no_sta_support = 1;
  }
  return 0;
}

/* add_subfeature_showtechfeature
 *
 * Show Tech Feature Flag will be used( in future ) to determine whether to
 * execute the subfeature as part of show tech feature command or not.  This
 * function sets this flag according to the configuration
 *
 * returns 0 on success and -1 on failure
 */

static int
add_subfeature_showtechfeature (struct sub_feature* element,
  const char* showtechfeature)
{
  if (element == NULL)
  {
    return -1;
  }
  if ((!strcmp_with_nullcheck ("yes", showtechfeature)) || (!strcmp_with_nullcheck ("true", showtechfeature)))
  {
    element->support_stb = 1;
  }
  else
  {
    element->support_stb = 0;
  }
  return 0;
}

 /* add_clicmds
  *
  * Adds new CliCmd Element after the given node, stores the cli command in
  * the command member of the node
  *
  * return pointer to the node on sucess and NULL on Error
  */

static struct clicmds*
add_clicmds (struct clicmds* afternode, const char* command)
{
  struct clicmds* element = NULL;
  element = (struct clicmds*) calloc (1, sizeof(struct clicmds));
  if (element == NULL)
  {
    VLOG_ERR ("Memory allocation failure\n");
    return NULL;
  }
  if(element->command != NULL)
  {
    free(element->command);
    element->command = NULL;
  }
  element->command = strdup_with_nullcheck (command);
  if (afternode != NULL)
  {
    element->next = afternode->next;
    afternode->next = element;
  }
  return element;

}

 /* add_ovstable
  *
  * Adds new ovstable Element after the given node
  *
  * return pointer to the node on sucess and NULL on Error
  */

static struct ovstable*
add_ovstable (struct ovstable* afternode)
{
  struct ovstable* elem = NULL;
  elem = (struct ovstable*) calloc (1, sizeof(struct ovstable));
  if (elem == NULL)
  {
    VLOG_ERR ("Memory allocation failure\n");
    return NULL;
  }
  if (afternode != NULL)
  {
    elem->next = afternode->next;
    afternode->next = elem;
  }
  return elem;
}

/* add_ovstable_name
 *
 * Adds ovstable name to the given table element
 *
 * returns 0 on success and -1 on failure
 */

static int
add_ovstable_name (struct ovstable* element, const char* tablename)
{
  if (element == NULL)
  {
    return -1;
  }
  if(element->tablename != NULL)
  {
    free(element->tablename);
    element->tablename = NULL;
  }
  element->tablename = strdup_with_nullcheck (tablename);

  return 0;
}

/* add_ovscolumn
 *
 * Adds new ovscolumn Element after the given node
 *
 * return pointer to the node on sucess and NULL on Error
 */

static struct ovscolm*
add_ovscolumn (struct ovscolm* afternode, const char* name)
{
  struct ovscolm* element = NULL;
  element = (struct ovscolm*) calloc (1, sizeof(struct ovscolm));
  if (element == NULL)
  {
    VLOG_ERR ("Memory allocation failure\n");
    return NULL;
  }
  if(element->name != NULL)
  {
    free(element->name);
    element->name = NULL;
  }
  element->name = strdup_with_nullcheck (name);
  if (afternode != NULL)
  {
    element->next = afternode->next;
    afternode->next = element;
  }
  return element;
}

/* list_features
 *
 * Function used for Internal and Unit Testing of Parser.
 * Displays the list of features parsed from the show tech yaml file
 */
void
list_features (struct feature* head)
{
  struct feature* iter = head;
  printf (" List of Supported Features \n");
  while (iter != NULL)
  {
    printf ("%s\t\t\t%s\n", iter->name, iter->desc);
    iter = iter->next;
  }
}


/* list_config
 *
 * Function used for Internal and Unit Testing of Parser.
 * Displays the entire Configuration parsed from the show tech yaml file
 * The output could be used to cross verify the configuration
 */
void
list_config (struct feature* head)
{
  struct feature* iter = head;
  struct sub_feature* iter_sub = NULL;
  struct clicmds* iter_cli = NULL;
  struct ovstable* iter_table = NULL;
  struct ovscolm* iter_col = NULL;
  printf (" Show Tech Configuration \n");
  while (iter != NULL)
  {
    printf ("\n\n%s\t\t\t%s\n", iter->name, iter->desc);
    iter_sub = iter->p_subfeature;

    while (iter_sub)
    {
      printf ("\nSub Feature \n");
      printf ("\t\t%s:%s:%d:%d\n", iter_sub->name, iter_sub->desc,
        iter_sub->no_sta_support, iter_sub->support_stb);

      printf ("\tCLI Commands:\n");
      iter_cli = iter_sub->p_clicmds;
      while (iter_cli)
      {
        printf ("\t\t%s\n", iter_cli->command);
        iter_cli = iter_cli->next;
      }
      iter_table = iter_sub->p_ovstable;
      printf ("\n\tOVS Tables:\n");
      while (iter_table)
      {
        printf ("\n\t\t%s:\n\t\t\t", iter_table->tablename);
        iter_col = iter_table->p_colmname;
        while (iter_col)
        {
          printf ("%s,", iter_col->name);
          iter_col = iter_col->next;
        }
        iter_table = iter_table->next;
      }
      iter_sub = iter_sub->next;
    }
    iter = iter->next;
  }

}

/* free_feature_data
 *
 * Function used to free the Show Tech Data structure which was used to
 * hold the show tech configuration.
 * This function is also responsible to reset the head pointer

 */

static void
free_feature_data ()
{
  struct feature* iter = feature_head;
  struct sub_feature* iter_sub = NULL;
  struct clicmds* iter_cli = NULL;
  struct ovstable* iter_table = NULL;
  struct ovscolm* iter_col = NULL;
  void* tempptr = NULL;
  feature_head = NULL;
  while (iter != NULL)
  {
      /* Free Feature Specific Data */
    free (iter->name);
    iter->name = NULL;
    free (iter->desc);
    iter->desc = NULL;

    iter_sub = iter->p_subfeature;

    while (iter_sub)
    {
          /* Free Sub Feature Specific Data */
      free (iter_sub->name);
      iter->name = NULL;

      free (iter_sub->desc);
      iter->desc = NULL;

      iter_cli = iter_sub->p_clicmds;
      while (iter_cli)
      {
        free (iter_cli->command);
        iter_cli->command = NULL;
        tempptr = (void*) iter_cli;
        iter_cli = iter_cli->next;
        free (tempptr);
        tempptr = NULL;
      }
      iter_table = iter_sub->p_ovstable;

      while (iter_table)
      {
        free (iter_table->tablename);
        iter_table->tablename = NULL;

        iter_col = iter_table->p_colmname;
        while (iter_col)
        {
          free (iter_col->name);
          iter_col->name = NULL;
          tempptr = (void*) iter_col;
          iter_col = iter_col->next;
          free (tempptr);
          tempptr = NULL;
        }
        tempptr = iter_table;
        iter_table = iter_table->next;
        free (tempptr);
        tempptr = NULL;
      }
      tempptr = iter_sub;
      iter_sub = iter_sub->next;
      free (tempptr);
      tempptr = NULL;
    }
    tempptr = iter;
    iter = iter->next;
    free (tempptr);
    tempptr = NULL;
  }
}

/* free_show_tech_config
 *
 * Function used to Free Show Tech Configuration.
 * All necessary datastructure and other free activities will be done here
 * This is exported as an API to other modules
 */
void
free_show_tech_config ()
{
  free_feature_data ();
}

/* parse_showtech_config
 *
 * Function used to parse the show tech configuration
 *
 * returns feature head on success and NULL on failure
 */
static struct feature*
parse_showtech_config (const char* config_file)
{
  FILE *fh = NULL;
  yaml_parser_t parser;
  yaml_event_t event; /* New variable */
  int event_value = 0;
  int current_state = 0;
  struct feature* curr_feature = NULL;
  struct sub_feature*curr_subfeature = NULL;
  struct clicmds* curr_clicmd = NULL;
  struct ovstable* curr_table = NULL;
  struct ovscolm* curr_col = NULL;
  const char* default_config_file = "/etc/openswitch/supportability/ops_showtech.yaml";
  const char* cf = NULL;
  VLOG_INFO("Show Tech configuration parser invoked");

  /* Free the Datastructure in use */
  free_show_tech_config ();
  /* Initialize parser */

  if (!yaml_parser_initialize (&parser))
  {
    VLOG_ERR ("Failed to initialize show tech parser");
    yaml_parser_delete (&parser);
    return NULL;
  }
  if(config_file)
  {
    cf = config_file;
  }
  else
  {
    cf = default_config_file;
  }
  fh = fopen (cf, "r");

  if (fh == NULL)
  {
    VLOG_ERR ("Failed to load Show Tech configuration file %s",cf);
    yaml_parser_delete (&parser);
    return NULL;
  }

  /* Set input file */
  yaml_parser_set_input_file (&parser, fh);

  /* START new code */
  if (!yaml_parser_parse (&parser, &event))
  {
    VLOG_ERR ("Parser error %d\n", parser.error);
    goto cleanup;
  }

  while (event.type != YAML_STREAM_END_EVENT)
  {

    if (event.type == YAML_SCALAR_EVENT)
    {
      event_value = getValueType ((const char*) event.data.scalar.value);
      switch (event_value)
      {
        case VALUE:
        {
          switch (current_state)
          {

            case FEATURE_NAME:
            {
              if (NULL == curr_feature)
              {
                VLOG_ERR ("Parsing error while adding new feature %s",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }
              add_feature_name (curr_feature,
                (const char*) event.data.scalar.value);
              break;
            }
            case FEATURE_DESC:
            {
              if (NULL == curr_feature)
              {
                VLOG_ERR (
                  "Parsing error while adding new feature description : %s",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }

              add_feature_desc (curr_feature,
                (const char*) event.data.scalar.value);
              break;
            }
            case SUB_FEATURE_NAME:
            {
              if (NULL == curr_subfeature)
              {
                VLOG_ERR (
                  "Parsing error while adding new sub feature %s",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }

              add_subfeature_name (
                curr_subfeature,
                (const char*) event.data.scalar.value);
              break;
            }
            case SUB_FEATURE_DESC:
            {
              if (NULL == curr_subfeature)
              {
                VLOG_ERR (
                  "Parsing error while adding new sub feature description :  %s",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }
              add_subfeature_desc (
                curr_subfeature,
                (const char*) event.data.scalar.value);
              break;
            }
            case SUPPORT_SHOWTECH_ALL:
            {
              if (curr_subfeature == NULL)
              {
                /* CLI Commands are added directly under Feature
                 * No SubFeature, Hence add a Dummy SubFeature */

                curr_subfeature = add_subfeature (curr_subfeature);
                if (NULL == curr_subfeature)
                {
                  VLOG_ERR (
                    "Parsing error while adding dummy sub feature ");
                  free_show_tech_config ();
                  goto cleanup;
                }
                set_subfeature_as_dummy(curr_subfeature);
                if (!curr_feature->p_subfeature)
                {
                  curr_feature->p_subfeature = curr_subfeature;
                }
                curr_clicmd = NULL;
                curr_table = NULL;
                curr_col = NULL;
              }
              add_subfeature_showtechall (
                curr_subfeature,
                (const char*) event.data.scalar.value);
              break;
            }
            case SUPPORT_SHOWTECH_FEATURE:
            {
              if (curr_subfeature == NULL)
              {
                          /* CLI Commands are added directly under Feature
                           * No SubFeature, Hence add a Dummy SubFeature */

                curr_subfeature = add_subfeature (curr_subfeature);
                if (NULL == curr_subfeature)
                {
                  VLOG_ERR (
                    "Parsing error while adding dummy sub feature ");
                  free_show_tech_config ();
                  goto cleanup;
                }
                set_subfeature_as_dummy(curr_subfeature);
                if (!curr_feature->p_subfeature)
                {
                  curr_feature->p_subfeature = curr_subfeature;
                }
                curr_clicmd = NULL;
                curr_table = NULL;
                curr_col = NULL;
              }
              add_subfeature_showtechfeature (
                curr_subfeature,
                (const char*) event.data.scalar.value);
              break;
            }
            case CLI_CMDS:
            {

              if (curr_subfeature == NULL)
              {
               /* CLI Commands are added directly under Feature
                * No SubFeature, Hence add a Dummy SubFeature */

                curr_subfeature = add_subfeature (curr_subfeature);
                if (NULL == curr_subfeature)
                {
                  VLOG_ERR (
                    "Parsing error while adding dummy sub feature ");
                  free_show_tech_config ();
                  goto cleanup;
                }
                set_subfeature_as_dummy(curr_subfeature);
                if (!curr_feature->p_subfeature)
                {
                  curr_feature->p_subfeature = curr_subfeature;
                }
                curr_clicmd = NULL;
                curr_table = NULL;
                curr_col = NULL;
              }
              curr_clicmd = add_clicmds (
                curr_clicmd, (const char*) event.data.scalar.value);
              if (NULL == curr_clicmd)
              {
                VLOG_ERR (
                  "Parsing error while adding cli command ");
                free_show_tech_config ();
                goto cleanup;
              }
              if (curr_subfeature->p_clicmds == NULL)
              {
                curr_subfeature->p_clicmds = curr_clicmd;
              }
              break;
            }
            case OVSDB:
            {
              break;
            }
            case TABLE_NAME:
            {
              if (NULL == curr_table)
              {
                VLOG_ERR ("Parsing error while adding table %s ",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }
              add_ovstable_name (curr_table,
               (const char*) event.data.scalar.value);
              break;
            }
            case COLNAMES:
            {
              if (NULL == curr_table)
              {
                VLOG_ERR ("Parsing error while adding column %s ",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }
              curr_col = add_ovscolumn (
                curr_col, (const char*) event.data.scalar.value);
              if (NULL == curr_col )
              {
                VLOG_ERR ("Parsing error while adding column %s ",
                  (const char*) event.data.scalar.value);
                free_show_tech_config ();
                goto cleanup;
              }
              if (curr_table->p_colmname == NULL)
              {
                curr_table->p_colmname = curr_col;
              }
              break;
            }
            default:
            {
              VLOG_ERR ("Unexpected state : %d %s", current_state,
                keystr[current_state]);
            }

          }
          break;
        }
        case FEATURE:
        {
          current_state = FEATURE;
          curr_clicmd = NULL;
          curr_subfeature = NULL;
          curr_table = NULL;
          curr_col = NULL;
          curr_feature = add_feature (curr_feature);
          if (NULL == curr_feature)
          {
            VLOG_ERR ("Parsing error while adding new feature");
            free_show_tech_config ();
            goto cleanup;
          }
          if (!feature_head)
          {
            feature_head = curr_feature;
          }

          break;
        }
        case FEATURE_NAME:
        {
          current_state = FEATURE_NAME;
          break;
        }
        case FEATURE_DESC:
        {
          current_state = FEATURE_DESC;

          break;
        }
        case SUB_FEATURE:
        {
          current_state = SUB_FEATURE;
          curr_clicmd = NULL;
          curr_table = NULL;
          curr_col = NULL;
          if (NULL == curr_feature)
          {
            VLOG_ERR ("Parsing error while creating new subfeature ");
            free_show_tech_config ();
            goto cleanup;
          }
          curr_subfeature = add_subfeature (curr_subfeature);
          if (NULL == curr_subfeature)
          {
            VLOG_ERR ("Parsing error while adding new subfeature");
            free_show_tech_config ();
            goto cleanup;
          }

          if (!curr_feature->p_subfeature)
          {
            curr_feature->p_subfeature = curr_subfeature;
          }

          break;
        }
        case SUB_FEATURE_NAME:
        {
          current_state = SUB_FEATURE_NAME;
          break;
        }
        case SUB_FEATURE_DESC:
        {
          current_state = SUB_FEATURE_DESC;
          break;
        }
        case SUPPORT_SHOWTECH_ALL:
        {
          current_state = SUPPORT_SHOWTECH_ALL;
          break;
        }
        case SUPPORT_SHOWTECH_FEATURE:
        {
          current_state = SUPPORT_SHOWTECH_FEATURE;
          break;
        }
        case CLI_CMDS:
        {
          current_state = CLI_CMDS;
          break;
        }
        case OVSDB:
        {
          current_state = OVSDB;
          break;
        }
        case TABLE:
        {

          if (NULL == curr_feature)
          {
            VLOG_ERR ("Parsing error while creating new table ");
            free_show_tech_config ();
            goto cleanup;
          }
          current_state = TABLE;
          curr_col = NULL;
          if (curr_subfeature == NULL)
          {
            curr_subfeature = add_subfeature (curr_subfeature);

            if (NULL == curr_subfeature)
            {
              VLOG_ERR (
                "Parsing error while adding dummy sub feature ");
              free_show_tech_config ();
              goto cleanup;
            }
            set_subfeature_as_dummy(curr_subfeature);

            if (!curr_feature->p_subfeature)
            {
              curr_feature->p_subfeature = curr_subfeature;
            }
            curr_clicmd = NULL;
            curr_table = NULL;

          }

          curr_table = add_ovstable (curr_table);

          if (NULL == curr_table)
          {
            VLOG_ERR (
              "Parsing error while adding new table ");
            free_show_tech_config ();
            goto cleanup;
          }
          if (curr_subfeature->p_ovstable == NULL)
          {
            curr_subfeature->p_ovstable = curr_table;
          }
          break;
        }
        case TABLE_NAME:
        {
          current_state = TABLE_NAME;
          break;
        }
        case COLNAMES:
        {
          current_state = COLNAMES;
          break;
        }
      }
    }
    if (event.type != YAML_STREAM_END_EVENT)
      yaml_event_delete (&event);
    if (!yaml_parser_parse (&parser, &event))
    {
      VLOG_ERR ("Parser error %d\n", parser.error);
      free_show_tech_config ();
      goto cleanup;
    }
  }

cleanup:

  yaml_event_delete (&event);
  yaml_parser_delete (&parser);
  fclose (fh);
  return feature_head;
}

/* get_showtech_config
 *
 * External API to expose the Show Tech Configuration datastructure
 * Currently used by CLI to find the list of commands to be executed under
 * various show tech features
 *
 * returns feature head on Success and NULL on failure
 */
struct feature*
get_showtech_config(const char* config_file)
{
  if(feature_head == NULL)
  {
    feature_head = parse_showtech_config(config_file);
  }
  return feature_head;
}
