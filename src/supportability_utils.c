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
 * Source file for the supportability common utils
 ***************************************************************************/


#include "supportability_utils.h"
#include "supportability_vty.h"

#define REGEX_COMP_ERR        1000


/* Function        : strncmp_with_nullcheck
 * Responsibility  : Ensure arguments are not null before calling strncmp
 * Return          : -1 if arguments are null otherwise return value from
 *                    strncmp
 */
int
strncmp_with_nullcheck( const char * str1, const char * str2, size_t num )
{
  if(str1 == NULL || str2 == NULL)
    return -1;
  return strncmp(str1,str2,num);
}





/* Function        : strcmp_with_nullcheck
 * Responsibility  : Ensure arguments are not null before calling strcmp
 * Return          : -1 if arguments are null otherwise return value from
 *                   strcmp
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


/* Helper function to trim white space around the core dump folder location */
char *
trim_white_space(char *string)
{
   char *endptr;
   char *beginptr = string;

   if(string == NULL)
   {
      return NULL;
   }
   /* Remove the white spaces at the beginning */
   while (isspace( (unsigned char)(*beginptr)))
   {
      beginptr++;
   }
   /* if the string contains only whitespace character */
   if(*beginptr == 0)
   {
      return beginptr;
   }

   /* Move the terminating null character next to the last non
      whitespace character */
   endptr = beginptr + strlen(beginptr) - 1;
   while(endptr > beginptr && (isspace( (unsigned char)(*endptr))) )
   {
      endptr--;
   }

   /* endptr points to the last valid entry, now the next entry should be
      terminating null character */
   *(endptr+1) = 0;
   return beginptr;
}

/* Helper function to compile the regex pattern */
int
compile_corefile_pattern (regex_t * regexst, const char * pattern)
{
   int status = regcomp (regexst, pattern, REG_EXTENDED|REG_NEWLINE);
   if (status != 0)
   {
      char error_message[REGEX_COMP_ERR];
      regerror (status, regexst, error_message, REGEX_COMP_ERR);
      printf ("Error in CoreDumpPattern '%s': %s\n",
            pattern, error_message);
      return -1;
   }
   return 0;
}

/* Function       : sev_level
 * Responsibility : To convert severity strings to values
 * return         : -1 if failed, otherwise severity value
 */
int
sev_level(char *arg)
{
    const char *sev[] = {"emer","alert","crit","err","warn","notice","info","debug"};
    int i = 0, found = 0;
    for(i = 0; i < MAX_SEVS; i++)
    {
        if(!strcmp_with_nullcheck(arg, sev[i])) {
            found = 1;
            break;
        }
    }
    if(found) {
        return i;
    }
    return -1;
}

/* Function       : get_values
 * Responsibility : read only values from keys
 * return         : return value
 */

const char*
get_value(const char *str)
{
   if(!str) {
      return NULL;
   }
   while(*str!='\0')
   {
      /*found the split*/
      if(*str == '=')  {
          if(*(str+1))  {
             /*value is present*/
               return str+1;
           }
          return NULL;
      }
      str++;
   }
return NULL;
}
