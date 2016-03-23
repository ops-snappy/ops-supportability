/*
 Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
 All Rights Reserved.

    Licensed under the Apache License, Version 2.0 (the "License"); you may
    not use this file except in compliance with the License. You may obtain
    a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
    License for the specific language governing permissions and limitations
    under the License.
*/

/*************************************************************************//**
 * @ingroup ops_supportability
 * This module contains the DEFINES and functions that comprise the event log
 * part of supportability library.
 *
 * @file
 * Source file for eventlog part of supportability library.
 *
 ****************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include "eventlog.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <systemd/sd-journal.h>
#include <yaml.h>
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(eventlog);

static event *ev_table = NULL;
static char *category_table[MAX_CATEGORIES_PER_DAEMON];
static int category_index = 0;


/* Function        : strcmp_with_nullcheck
* Responsibility  : Ensure arguments are not null before calling strcmp
* Return          : -1 if arguments are null otherwise return value form strcmp
*/
int
strcmp_with_nullcheck( const char * str1, const char * str2 )
{
  if(str1 == NULL || str2 == NULL)
    return -1;
  return strcmp(str1, str2);
}

/* count_keys
 * Counts the number of keys in the event string
 *
 * Returns - 1 on failure, number of keys on success.
 */
int
count_keys(char *keys)
{
    char *key_token = NULL;
    int key_count = 0;
    if(keys == NULL) {
       return -1;
    }
    key_token = strtok(keys, ",");
    while(key_token != NULL)
    {
        key_token = strtok(NULL, ",");
        key_count++;
    }
    return key_count;
}

/* assign_parsed_values
 * assigns the values from yaml file passed to indexed
 * location in event table.
 *
 * Returns none
 */
void
assign_parsed_values(char *key, int *val, int *fnd, int *index)
{
    int tmp = *index;
    int size = 0;
    switch(*val) {

        case 1:
            *val = 0;
            break;

        case 2:
            ev_table[tmp].event_id = atoi(key);
            *val = 0;
            break;

        case 3:
            size = strlen(key);
            if((size > 0) && (size < MAX_SEV_NAME_SIZE )) {
                strncpy(ev_table[tmp].severity, key, (size+1));
            }
            *val = 0;
            break;

        case 4:
            ev_table[tmp].num_of_keys = count_keys(key);
            *val =0;
            break;

        case 5:
            size = strlen(key);
            if((size > 0) && (size < MAX_LOG_STR)) {
                strncpy(ev_table[tmp].event_description, key, (size+1));
            }
            *val = 0;
            *fnd = 0;
            (*index)++;
            strncpy(ev_table[tmp+1].event_name, EVENT_NAME_DELIMITER_STR,
            (strlen(EVENT_NAME_DELIMITER_STR)+1));
            break;
    }
}

/* find_last_index
 * Searches for the delimiter in event table to find
 * the next index to fill
 *
 * Returns index on success, -1 on failure.
 */
int
find_last_index()
{
    int i = 0;
    /* Loop & figure out the index to append
     * in ev_table */
    while(i < MAX_EVENT_TABLE_SIZE)
    {
        if(!strcmp_with_nullcheck(ev_table[i].event_name,
            EVENT_NAME_DELIMITER_STR)) {
            return i;
        }
        i++;
    }
   return -1;
}

/* parse_yaml_for_category
 * Parses the events.yaml file for the events with
 * the category passed.
 *
 * Returns 1 if atleast an event with the category is found
 * on failure returns -1.
 */
int
parse_yaml_for_category(char *category)
{
    yaml_parser_t parser;
    yaml_token_t token;
    char buf[MAX_EVENT_NAME_SIZE] = {0,};
    char *key = NULL;
    int index = 0, ret = 0;
    int def_flag=0,evt_flag=0,val_flag=0;
    int found = 0, category_found = 0, size = 0;
    FILE* fh;

    if(category == NULL) {
        return -1;
    }
    fh = fopen(EVENT_YAML_FILE, "r");
    if(fh == NULL) {
        VLOG_ERR("YAML file open failed");
        return -1;
    }
    index = find_last_index();
    if((index < 0) || (index > MAX_EVENT_TABLE_SIZE)) {
        return -1;
    }
    if (!yaml_parser_initialize(&parser)) {
        VLOG_ERR("YAML Initialize failed");
        return -1;
    }
    yaml_parser_set_input_file(&parser, fh);
    /* Lets loop through all tokens & assign the values
     * in ev_table index of the given event */
    while(token.type!= YAML_STREAM_END_TOKEN)
    {
        yaml_parser_scan(&parser, &token);
        switch(token.type)
        {
            case YAML_KEY_TOKEN:
                break;

            case YAML_VALUE_TOKEN:
                break;

            case YAML_SCALAR_TOKEN:
                key= (char*)token.data.scalar.value;
                if(key == NULL) {
                    continue;
                }

                if(val_flag == 1)
                {
                    size = strlen(key);
                    if((size > 0) && (size < MAX_EVENT_NAME_SIZE)) {
                        strncpy(buf, key, (size+1));
                    }
                    val_flag = 0;

                }
                if((evt_flag) && (!strcmp(key, category)))
                {
                    found = 1;
                    category_found = 1;
                    size = strlen(buf);
                    if((size > 0) && (size < MAX_EVENT_NAME_SIZE)) {
                        strncpy(ev_table[index].event_name, buf, (size+1));
                    }
                }
                if(found)
                {
                    assign_parsed_values(key, &val_flag, &found, &index);
                    ret = asprintf(&ev_table[index].category, "%s", category);
                    if(ret < 0) {
                        return -1;
                        VLOG_ERR("Failed to allocate memory");
                    }
                }
                if(!strcmp_with_nullcheck(key, "event_definitions"))
                {
                    def_flag=1;
                    continue;
                }
                else if((!strcmp_with_nullcheck(key, "event_name"))
                     && (def_flag))
                {
                    val_flag=1;
                    continue;

                }
                else if((!strcmp_with_nullcheck(key, "event_category"))
                    && (def_flag))
                {
                    evt_flag =1;
                    continue;
                }
                else if((!strcmp_with_nullcheck(key, "event_ID"))
                    && (def_flag))
                {
                    val_flag=2;
                    continue;

                }
                else if((!strcmp_with_nullcheck(key, "severity"))
                    && (evt_flag))
                {
                    val_flag=3;
                    continue;
                }
                else if((!strcmp_with_nullcheck(key, "keys"))
                    && (evt_flag))
                {
                    val_flag=4;
                    continue;

                }
                else if((!strcmp_with_nullcheck(key, "event_description_template"))
                    && (evt_flag))
                {
                    val_flag =5;
                    continue;
                }
                break;

                default: break;
            }
            if (token.type != YAML_STREAM_END_TOKEN) {
                yaml_token_delete(&token);
            }
        }
    yaml_parser_delete(&parser);
    fclose(fh);
    return category_found;
}

/* create_event_table
 * Creates the event ID table.
 *
 * Returns 0 on success, -1 on failure.
 */
int
create_event_table()
{
    ev_table = (event*)malloc((MAX_EVENT_TABLE_SIZE*sizeof(event)));
    if(ev_table == NULL) {
        return -1;
    }
    /* Lets fill in the delimiter at 1st index to show that
     * table is empty */
    strncpy(ev_table[0].event_name, EVENT_NAME_DELIMITER_STR,
    (strlen(EVENT_NAME_DELIMITER_STR)+1));
    return 0;
}

/* add_to_event_table
 * Add the events belonging to the category to
 * event table
 *
 * Returns the return value returned by the yaml parser
 * API.
 */
int
add_to_event_table(char *event_category)
{
    int ret = 0;
    ret = parse_yaml_for_category(event_category);
    return ret;
}

/* event_category_search
 * Searches the category table for the given category
 *
 * Returns TRUE(1) if category is found, else FALSE
 */
int
event_category_search(char *category)
{
    int i = 0;
    if(category == NULL) {
        return FALSE;
    }
    /* Loop till the existing last index to find the category */
    while(i < category_index)
    {
        if(!strcmp_with_nullcheck(category_table[i], category)) {
            return TRUE;
        }
        else {
            i++;
        }
    }
    return FALSE;
}

/* event_log_init
 * Initialization function for event log for daemon.
 * creates daemon event table with category of interest
 *
 * Returns 0 on success, -1 on failure
 */
int
event_log_init(char *category_name)
{
    int ret = 0;
    if(category_name == NULL) {
        return -1;
    }

    if(category_index > (MAX_CATEGORIES_PER_DAEMON)) {
        return -1;
    }
    if(category_index) {
        /* Lets check whether event_log_init() on this category
         * was already done. If that is the case it will be present
         * in category table we maintain */
        if(event_category_search(category_name)) {
            return -1;
        }
    }
    if(ev_table == NULL) {
        /* It seems this is the first call of event_log_init()
         * by this daemon, lets create the daemon event table then
         */
        ret = create_event_table();
        if(ret < 0) {
            return -1;
        }
    }
    /* Lets add the events belonging to this category to event table */
    ret = add_to_event_table(category_name);
    if(ret > 0) {
        /* Add this category to category table, so that next time
         * if somebody calls with the same category we will not
         * duplicate all this work we already did */
        asprintf(&category_table[category_index], "%s", category_name);
        category_index++;
    }
    return ret;
}

/* key_value_string
 * Forms the string in "key=value" format
 *
 *returns formed string on sucess, NULL on failure.
 */
char
*key_value_string(char *s1, ...)
{
    char *tmp = NULL;
    int size = 0;
    char str[KEY_VALUE_SIZE] = {0,};
    char str2[KEY_VALUE_SIZE] = {0,};
    /* This memory is freed in log_event():line 622 */
    char *kv_pair = (char*)malloc(KEY_VALUE_SIZE);
    if(kv_pair == NULL) {
        return NULL;
    }
    va_list arg;
    va_start(arg, s1);
    size = strlen(s1);
    if((size > 0) && (size < KEY_VALUE_SIZE)) {
        strncpy(str, s1, (size+1));
    }
    else {
        return NULL;
    }
    /* Add "=" to make "key=value" string */
    tmp = strcat(str, "=");
    s1 = va_arg(arg, char*);
    vsnprintf(str2, KEY_VALUE_SIZE, s1, arg);
    size = strlen(str2);
    if((size > 0) && (size < KEY_VALUE_SIZE)) {
        strcat(tmp, str2);
    }
    size = strlen(tmp);
    if((size > 0) && (size < KEY_VALUE_SIZE)) {
        strncpy(kv_pair, tmp, (size+1));
    }
    va_end(arg);
    return kv_pair;
}

/* make_key_format
 * Make key name string in format "{KEY}"
 *
 * returns 0 on success, -1 on failure.
 */
int
make_key_format(char *key)
{
    int len = 0, i = 0;
    if(key == NULL) {
        return -1;
    }
    len = strlen(key);
    if(len > (KEY_VALUE_SIZE-2)) {
        VLOG_ERR("Key name too large");
        return -1;
    }

    /* Move over! */
    for (i = len; i >= 0; i--)
    {
        key[i + 1] = key[i];
    }

    /* Now plug in the new prefix. */
    key[0] = '{';
    strncat(key, "}", len);
    return 0;
}

/* replace_str
 * Search & replace the provided key in the string
 * with value
 *
 * returns the replaced string on success.
 */
char
*replace_str(char *str, char *orig, char *rep)
{
    static char buffer[MAX_LOG_STR] = {0,};
    char *p = NULL;
    char key_name[(strlen(orig)+2)];
    int size = 0;
    if(orig != NULL) {
        size = strlen(orig);
    }
    else {
        return NULL;
    }
    memset(key_name, 0, sizeof(key_name));
    strncpy(key_name, orig, (size+1));
    /* Prepend & append '}' to key name */
    if(make_key_format(key_name) < 0) {
        return NULL;
    }
    /* Loop till we replace all keys with same name in str */
    while((p = strstr(str, key_name)) != NULL)
    {
        if(str != NULL) {
            strncpy(buffer, str, p-str);
        }
        else {
            return NULL;
        }
        buffer[p-str] = '\0';
        sprintf(buffer+(p-str), "%s%s", rep, p+strlen(key_name));
        size = strlen(buffer);
        if(str != NULL) {
            strncpy(str, buffer, (size+1));
        }
        else {
            return NULL;
        }
    }

    return buffer;
}

/* populate_str
 * Populates the given string with the key value pairs.
 *
 * Returns 0 on sucess, -1 on failure.
 */
int
populate_str(char *s1, char *s2)
{
    char *prev = NULL, *token = NULL;
    char *string = NULL;
    int size = 0;
    char *final = NULL;
    if ((s1 == NULL) || (s2 == NULL)) {
        return -1;
    }
    size = strlen(s2);
    if((size > 0) && (size < KEY_VALUE_SIZE)) {
        size = asprintf(&string, "%s", s2);
        if(size < 0) {
            return -1;
        }
    }
    else {
        return -1;
    }
    token = strtok(string, "=");
    prev = token;
    while(token != NULL)
    {
        token = strtok(NULL, "=");
        if(token != NULL)
        final = replace_str(s1, prev, token);
        if(final == NULL) {
            return -1;
        }
    }
    size = strlen(final);
    if((size > 0) && (size < MAX_LOG_STR)) {
        strncpy(s1, final, (size+1));
    }
    else {
        free(string);
        return -1;
    }
    free(string);
    return 0;
}

/* event_search
 * Searches the event table for the given event
 *
 * Returns event index on success, -1 on failure.
 */
int
event_search(char *fmt)
{
    int i = 0;
    if(fmt == NULL) {
        return -1;
    }
    /* Loop till we either match event name or
     * we reach the delimter which is EV_TBD_TBD */
    while(i < MAX_EVENT_TABLE_SIZE)
    {
        if(!strcmp_with_nullcheck(fmt, ev_table[i].event_name)) {
            return i;
        }
        if(!strcmp_with_nullcheck(EVENT_NAME_DELIMITER_STR,
                    ev_table[i].event_description)) {
            break;
        }
        i++;
    }
    return i;
}

/* severity_level
 * To convert severity string to severity value.
 *
 * Returns -1 on failure & severity value on success
 */
int
severity_level(char *arg)
{
    const char *sev[] = {"LOG_EMERG","LOG_ALERT","LOG_CRIT","LOG_ERR",
                         "LOG_WARN","LOG_NOTICE","LOG_INFO","LOG_DEBUG"};
    int i, found = 0;
    for(i = 0; i < MAX_SEV_LEVELS; i++)
    {
        if(!strcmp_with_nullcheck(arg, sev[i])) {
            found = TRUE;
            break;
        }
    }
    if(found) {
        return i;
    }
    return -1;
}

/* log_event
 * API used to log the event logs.
 *
 * Returns -1 on failure & 0 on success
 */
int
log_event(char *ev_name,...)
{
    int i = 0, index = 0, key_nums = 0, key_value_none = 0;
    int ret = 0, str_size = 0;
    va_list arg;
    char key_value_pair[KEY_VALUE_SIZE] = {0,};
    char all_key_value_pairs[(2*KEY_VALUE_SIZE)] = {0,};
    char *tmp = NULL;
    char *message = NULL;
    char evt_msg[MAX_LOG_STR] = {0,};
    int level = 0;
    if(ev_name == NULL) {
        return -1;
    }
    va_start(arg, ev_name);
    /* Search for the event in event table
     * Fetch it's index */
    index = event_search(ev_name);
    if((index == (MAX_EVENT_TABLE_SIZE-1)) || (index < 0))
    {
        ret = sd_journal_send("ops-evt|Unknown Event Name %s", ev_name,
                "MESSAGE_ID=%s", MESSAGE_OPS_EVT,
                NULL);
        if(ret != 0) {
            VLOG_ERR("sd_journal_send failed with %d", ret);
        }
        return -1;
    }
    str_size = strlen(ev_table[index].event_description);
    if(str_size < MAX_LOG_STR) {
        strncpy(evt_msg, ev_table[index].event_description, (str_size+1));
    }
    /* Get the number of key's in the event */
    key_nums = ev_table[index].num_of_keys;
    while(i < key_nums)
    {
        tmp = va_arg(arg, char*);
        if(tmp == NULL)
        {
            /* this means we don't have key-value pair at all!
             * so let's break & call journal API with just message.
             */
            key_value_none = 1;
            break;
        }
        str_size = strlen(tmp);
        if(str_size < KEY_VALUE_SIZE) {
            strncpy(key_value_pair, tmp, (str_size+1));
        }
        /* Populate the key with the value in message */
        ret = populate_str(evt_msg, key_value_pair);
        if(ret < 0) {
            VLOG_ERR("Failure at populate_str()");
            return -1;
        }
        /* Make all the key-value pair's in the form of
         * key1=value1,key2=value,... format to pass to
         * journal API */
        strcat(key_value_pair, ",");
        strncat(all_key_value_pairs, key_value_pair,
        (sizeof(all_key_value_pairs)-strlen(all_key_value_pairs)-1));
        i++;
        free(tmp);
    }
    ret = asprintf(&message, "MESSAGE=ops-evt|%d|%s|%s",
            ev_table[index].event_id, ev_table[index].severity, evt_msg);
    if(ret < 0) {
        VLOG_ERR("Failed to allocate memory");
        return -1;
    }
    /* Convert severity string to corresponding severity value */
    level = severity_level(ev_table[index].severity);
    if(level < 0) {
        VLOG_ERR("Incorrect severity level");
        return -1;
    }
    if(key_value_none) {
        ret = sd_journal_send(message, "PRIORITY=%d", level,
                "MESSAGE_ID=%s", MESSAGE_OPS_EVT,"OPS_EVENT_ID=%d",
                ev_table[index].event_id,"OPS_EVENT_CATEGORY=%s",
                 ev_table[index].category, NULL);
    }
    else {
        ret = sd_journal_send(message, "PRIORITY=%d", level,
                "MESSAGE_ID=%s", MESSAGE_OPS_EVT,"OPS_EVENT_ID=%d",
                ev_table[index].event_id, "OPS_EVENT_CATEGORY=%s",
                ev_table[index].category,
                all_key_value_pairs,
                NULL);
    }
    if(ret != 0) {
        VLOG_ERR("sd_journal_send failed with %d", ret);
    }
    free(message);
    return ret;
}
