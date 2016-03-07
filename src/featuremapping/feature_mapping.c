/* Feature to Daemon Mapping Code
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
 * File: feature_mapping.c
 *
 * Purpose: To map Feature name to Daemon name
 */

#include "feature_mapping.h"
#include <yaml.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(feature_mapping);

static int
parse_feature_mapping_yaml(void);

static int
feature_mapping_check_key(const char *data);

static struct daemon*
feature_mapping_add_daemon( struct daemon* afternode,
                            const char* daemon);
static int
feature_mapping_add_feature_desc( struct feature* element,
                                  const char* feature_desc);

static struct feature*
feature_mapping_add_feature(struct feature* afternode,
                            const char* feature_name);

int
strcmp_with_nullcheck( const char *, const char *);

const char* keystr_feat[MAX_NUM_KEYS] = {
  "values",
  "feature_name",
  "feature_desc",
  "daemon",
};

static struct feature* feature_head = NULL;


/*
* Function       : feature_mapping_add_feature
* Responsibility : create and add a node in linked list . New node contains
*                  feature name string
*
* Parameters
*                : feature_name - name of feature
*                : afternode - new nodes next pointer will point to this
*
* Returns        : NULL on failure
*                  Pointer to new node on sucess
*/

static struct feature*
feature_mapping_add_feature(struct feature* afternode, const char* feature_name)
{
  struct feature* elem = NULL;
  elem = (struct feature*)calloc(1,sizeof(struct feature));
  if(elem == NULL) {
    VLOG_ERR("Memory Allocation Failure\n");
    return NULL;
  }
  if(feature_name == NULL) {
     return NULL;
  }
  elem->name = strndup(feature_name,FEATURE_SIZE);
  if(afternode != NULL) {
    elem->next = afternode->next;
    afternode->next = elem;
  }
  return elem;
}

/*
* Function       : feature_mapping_add_feature_desc
* Responsibility : add a feature node in linked list
* Parameters
*                : feature_name - name of feature
*                : afternode - new nodes next pointer will point to this node
*
* Returns        : 0 on scucess
*                  nonzero on failure
*/


static int
feature_mapping_add_feature_desc(
  struct feature* element,
  const char* feature_desc
)
{
  if(element == NULL) {
    return 1;
  }
  if(feature_desc == NULL) {
     return 1;
  } else {
  element->desc = strndup(feature_desc,FEATURE_SIZE);
  }
  return 0;
}

  /*
  * Function       : feature_mapping_add_daemon
  * Responsibility : add daemon name in feature linked list
  * Parameters
  *                : afternode - new nodes next pointer will point to this node
  *                : daemon - name of daemon
  * Returns        : void
  */

static struct daemon*
feature_mapping_add_daemon(
  struct daemon* afternode,
  const char* daemon
)
{
  struct daemon* elem = NULL;
  elem = (struct daemon*)calloc(1,sizeof(struct daemon));
  if(elem == NULL) {
    VLOG_ERR("Memory Allocation Failure\n");
    return NULL;
  }
  if(daemon == NULL) {
     return NULL;
  }
  elem->name = strndup(daemon,FEATURE_SIZE);
  if(afternode != NULL) {
    elem->next = afternode->next;
    afternode->next = elem;
  }
  return elem;

}

/*
* Function       : feature_mapping_check_key
* Responsibility : helper function to parse yaml file
* Parameters
*                : data
* Returns        : integer
*/

static int
feature_mapping_check_key(const char *data)
{
  int i = FEATURE_NAME;
  for( i = FEATURE_NAME; i < MAX_NUM_KEYS; i++) {
    if(!strcmp_with_nullcheck(keystr_feat[i],data)) {
      return i;
    }
  }
  /* Data didn't match any of the keywords, hence it should be value */
  return VALUE;
}

/*
* Function       : parse_feature_mapping_yaml
* Responsibility : parse feature to daemon mapping config file
*                  and store in linkedlist
* Parameters     : void
* Returns        : 0 on sucess and nonzero on failure
*/

static int
parse_feature_mapping_yaml(void)
{
    FILE *fh=NULL;
    yaml_parser_t parser;
    yaml_event_t  event;
    int event_value = 0;
    int current_state = 0;
    struct feature*   curr_feature    = NULL;
    struct daemon*    curr_daemon     = NULL;
    /* Initialize parser */
    if(!yaml_parser_initialize(&parser)) {
        VLOG_ERR("Failed to initialize parser!");
        return 1;
    }
    fh = fopen(FEATURE_MAPPING_CONF, "r");
    if(fh == NULL) {
        VLOG_ERR("Failed to open file :%s",FEATURE_MAPPING_CONF);
        yaml_parser_delete(&parser);
        return 1;
    }

    /* Set input file */
    yaml_parser_set_input_file(&parser, fh);

    /* START new code */
    if (!yaml_parser_parse(&parser, &event)) {
        VLOG_ERR("Parser error %d", parser.error);
        yaml_parser_delete(&parser);
        yaml_event_delete(&event);
        fclose(fh);
        return 1;
    }

    while(event.type != YAML_STREAM_END_EVENT){
        if(event.type == YAML_SCALAR_EVENT) {
            event_value = feature_mapping_check_key (
                    (const char*) event.data.scalar.value);
            switch(event_value) {
                case VALUE:
                    {
                        switch(current_state) {
                            case FEATURE_NAME:
                                {
                                    curr_daemon  = NULL;
                                    curr_feature = feature_mapping_add_feature(
                                            curr_feature,
                                            (const char*)
                                            event.data.scalar.value);
                                    if(!feature_head)
                                    {
                                        feature_head = curr_feature;
                                    }
                                    break;
                                }
                            case FEATURE_DESC:
                                {
                                    feature_mapping_add_feature_desc(
                                            curr_feature,
                                            (const char*)
                                            event.data.scalar.value);
                                    break;
                                }
                            case DAEMON:
                                {
                                    curr_daemon = feature_mapping_add_daemon(
                                            curr_daemon,
                                            (const char*)
                                            event.data.scalar.value);
                                    if(curr_feature->p_daemon == NULL)
                                    {
                                        curr_feature->p_daemon = curr_daemon;
                                    }
                                    break;
                                }

                            default:
                                break;

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
                case DAEMON:
                    {
                        current_state = DAEMON;
                        break;
                    }

                default:
                    break;
            }
        }
        if(event.type != YAML_STREAM_END_EVENT){
            yaml_event_delete(&event);
        }
        if (!yaml_parser_parse(&parser, &event)) {
            VLOG_ERR("Parser error %d\n", parser.error);
            yaml_parser_delete(&parser);
            yaml_event_delete(&event);
            fclose(fh);
            return 1;
        }
    }
    yaml_event_delete(&event);

    /* Cleanup */
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}

/*
* Function       : get_feature_mapping
* Responsibility : singleton function to get the feature mapping
* Parameters
*                : data
* Returns        : header to the feature mapping link list
*/

struct feature*
get_feature_mapping(void)
{
  if(feature_head == NULL)
  {
    parse_feature_mapping_yaml();
  }
  return feature_head;
}
