/* System SYSLOG CLI commands
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
* File: syslog_vty.c
*
* Purpose: To config syslog from CLI
*/

#include "vtysh/command.h"
#include "vtysh_ovsdb_if.h"
#include "vtysh_ovsdb_config.h"
#include "vtysh/vtysh.h"
#include "vtysh/vtysh_user.h"
#include "syslog_vty.h"
#include "vswitch-idl.h"
#include "ovsdb-idl.h"
#include "smap.h"
#include "vtysh/memory.h"
#include "openvswitch/vlog.h"
#include "dynamic-string.h"
#include "vtysh/buffer.h"
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "supportability_utils.h"
#include <termios.h>

VLOG_DEFINE_THIS_MODULE (vtysh_syslog_cli);
extern struct ovsdb_idl *idl;

#define VLOG_ERR_SYSLOG_TRANSACTION_COMMIT_FAILED \
VLOG_ERR("syslog remote : transaction commit failed \n")
#define VLOG_ERR_SYSLOG_INSERT_FAILED \
VLOG_ERR("syslog remote : inserting new row failed \n")
#define VLOG_ERR_SYSLOG_OPENVSWITCH_READ_FAILED \
VLOG_ERR("syslog remote : DB read failed \n")
#define VLOG_ERR_SYSLOG_TRANSACTION_CREATE_FAILED  \
VLOG_ERR(OVSDB_TXN_CREATE_ERROR)

#define DEFAULT_UDP_PORT            514
#define DEFAULT_TCP_PORT            1470
#define UDP_TRANS_PROTOCOL          0
#define TCP_TRANS_PROTOCOL          1
#define MAX_REMOTE_SERVERS          4
#define MIN_PORT_NUMBER_SUPPORTED   1
#define MAX_PORT_NUMBER_SUPPORTED   65535

#define HOST_ONLY_MATCH             1
#define NO_HOST_ONLY_MATCH          0
#define EXACT_MATCH                 1
#define SIMILAR_MATCH               0

#define DO_LOGGING                  1
#define DO_NO_LOGGING               0


/* Function          :  syslog_remote_get_config
 * Responsibility    :  Find if similar configuration exist in the OVSDB
 * Returns           :  row if similar configuration exist otherwise NULL
 */

/* This function is used in three different context
1. To find a record that matches the given remote_host alone.
   Usecase : To replace the existing configuration with the new
   configuration for the same remote_host.  Currently we support
   only one configuration per remote, hence we are replacing
   existing record with new one for the same remote host
   Condition : host_only_match is true
2. To find a similar record, ie., record that is matching all the
   user provided input.
   Usecase : To delete an existing configuration.
   Condition : host_only_match is false, exact_match is false
3. To find exact match, ie., record matching all the user provided
   input.
   Usecase :  Used by add configuration to check if the same
   configuration already exists
   Condition : host_only_match is false, exact_match is true
*/
const struct ovsrec_syslog_remote*
syslog_remote_get_config(const char* remote_host,
              const char* transport,
              const int64_t* port_number,
              const char* severity,
              int host_only_match,
              int exact_match)
{

    const struct ovsrec_syslog_remote *row= NULL;
    const struct ovsrec_syslog_remote *next= NULL;
    int curr_transport = UDP_TRANS_PROTOCOL;

    /* Browse through each record present in the OVSDB */
    OVSREC_SYSLOG_REMOTE_FOR_EACH_SAFE(row,next,idl)
    {
        /* Current Transport variable is used to determine the default port
        number, since we use 514 as default for UDP and 1470 as default for
        tcp, we need to know the transport currently used in this record */
        curr_transport = 0;

        /* Compare Host name */
        /* This is common for both use case 1 and 2 */
        if(strcmp_with_nullcheck(row->remote_host,remote_host))
        {
            continue;
        }

        /* Following code is used only by use case 2. exact match*/
        if(!host_only_match)
        {
            /* Compare Transport */
            /* If the user has provided input then perform comparision */
            if(transport != NULL)
            {
                if(row->transport != NULL)
                {
                    if(strcmp_with_nullcheck(row->transport,transport))
                    {
                        continue;
                    }
                }
                else
                {
                    /* Default is UDP, hence compare against it */
                    if(strcmp_with_nullcheck("udp",transport))
                    {
                        continue;
                    }
                }
            }
            else if (exact_match)
            {
               /* We need to find the exact row which matches the given input */
               /* For eg., consider the following row in OVSDB
                  logging 10.0.0.2 tcp 559
                  and in query if only remote_host is given, we should not match
                  since the default is udp where as the record in OVSDB is tcp */
                if(row->transport != NULL)
                {
                    if(strcmp_with_nullcheck(row->transport,"udp"))
                    {
                        continue;
                    }
                }

            }

            /* Set whether the transport protocol is udp or tcp
             * It is sufficient to check just row->transport for protocol,
             * since we would reach this point only if both matches. */
            if(row->transport == NULL ||
                    !strcmp_with_nullcheck(row->transport,"udp"))
            {
                curr_transport = UDP_TRANS_PROTOCOL;
            }
            else
            {
                curr_transport = TCP_TRANS_PROTOCOL;
            }


            /* Compare Port_Number*/
            /* If the user has provided port number, then compare */
            if(port_number != NULL)
            {
                if(row->port_number == NULL)
                {
                    if(curr_transport ==UDP_TRANS_PROTOCOL &&
                            *port_number != DEFAULT_UDP_PORT)
                    {
                        continue;
                    }
                    else if(curr_transport == TCP_TRANS_PROTOCOL &&
                            *port_number != DEFAULT_TCP_PORT)
                    {
                        continue;
                    }
                }
                else
                {
                    if(*row->port_number != *port_number)
                    {
                        continue;
                    }
                }
            }
            else if (exact_match)
            {
                if(row->port_number != NULL)
                {
                    if(curr_transport ==UDP_TRANS_PROTOCOL &&
                            *(row->port_number) != DEFAULT_UDP_PORT)
                    {
                        continue;
                    }
                    else if(curr_transport == TCP_TRANS_PROTOCOL &&
                            *(row->port_number) != DEFAULT_TCP_PORT)
                    {
                        continue;
                    }
                }
            }
            /* If both of them are NULL, then they match  */


            /* Compare severity only if provided by user */
            if(severity != NULL)
            {
                if(row->severity != NULL)
                {
                    if(strcmp(row->severity,severity))
                    {
                        continue;
                    }
                }
                else
                {
                    /* Compare with default severity */
                    if(strcmp_with_nullcheck("debug",severity))
                    {
                        continue;
                    }
                }
            }
            else if (exact_match)
            {
                if(row->severity != NULL)
                {
                    if(strcmp_with_nullcheck(row->severity,"debug"))
                    {
                        continue;
                    }
                }
            }
        }
        return row;
    }
    return NULL;
}

/* Function          :  cli_syslog_delete_config
 * Responsibility    :  Delete the matching OVSDB configuration
 * Returns           :  CMD Status
 */
int
cli_syslog_delete_config(const char* remote_host,
              const char* transport,
              const int64_t* port_number,
              const char* severity,
              const struct ovsrec_system *system_row,
              int host_only_match,
              int error_on_no_match
        )
{
    int i,n;
    const struct ovsrec_syslog_remote *row = NULL;
    struct ovsrec_syslog_remote **row_array = NULL;

    /* Check if a matching row exists in the db */
    row = syslog_remote_get_config(
            remote_host,transport,port_number,severity,
            host_only_match,SIMILAR_MATCH);

    /* If the config exists, then delete the row */
    if(row != NULL)
    {
        ovsrec_syslog_remote_delete(row);

        /* Remove its association from the parent table (system table) */
        /* Update the system table */
        row_array = (struct ovsrec_syslog_remote **)
            calloc(sizeof(struct ovsrec_syslog_remote *),
                    system_row->n_syslog_remotes-1);
        if(row_array == NULL)
        {
            VLOG_ERR_SYSLOG_INSERT_FAILED;
            return CMD_OVSDB_FAILURE;
        }

        for( i = n =0;i < system_row->n_syslog_remotes;i++ )
        {
            if(system_row->syslog_remotes[i] != row)
            {
                row_array[n++] = system_row->syslog_remotes[i];
            }
        }
        ovsrec_system_verify_syslog_remotes(system_row);
        ovsrec_system_set_syslog_remotes(system_row,
                row_array,
                n);
        free(row_array);
    }
    /* In case of no logging command we need to error out informing that the
       matching configuration doesn't exist */
    else if (error_on_no_match)
    {
        vty_out(vty,"Syslog configuration not found%s",VTY_NEWLINE);
        return CMD_WARNING;
    }
    return CMD_SUCCESS;
}


/* Function          :  cli_syslog_config
 * Responsibility    :  Updates the syslog_remote table based on the CLI
 *                      configuration provided by the user
 * Returns           :  CMD Status
 */
int64_t
cli_syslog_config(const char* remote_host,
              const char* transport,
              const int64_t* port_number,
              const char* severity,
              int is_add)
{
    int i = 0;
    int returncode;
    struct ovsrec_syslog_remote *row = NULL;
    struct ovsrec_syslog_remote **row_array = NULL;
    const struct ovsrec_system *system_row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *txn = cli_do_config_start();

    if(txn == NULL)
    {
        VLOG_ERR_SYSLOG_TRANSACTION_CREATE_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
    }

    if(remote_host == NULL)
    {
        vty_out(vty,"Command failed%s",VTY_NEWLINE);
        cli_do_config_abort(txn);
        return CMD_WARNING;
    }

    system_row = ovsrec_system_first(idl);
    if(system_row == NULL)
    {
        VLOG_ERR_SYSLOG_OPENVSWITCH_READ_FAILED;
        cli_do_config_abort(txn);
        return CMD_OVSDB_FAILURE;
    }


    /* In case of adding new configuration, if exact configuration already
     * exists then simply return
     */
    if( is_add )
    {
        if(NULL != syslog_remote_get_config(
            remote_host,transport,port_number,severity,
            NO_HOST_ONLY_MATCH,EXACT_MATCH))
        {
            /* Exact configuration already exist, no change */
            VLOG_DBG("Syslog: Exact configuration already exists");
            cli_do_config_abort(txn);
            return CMD_SUCCESS;
        }
    }

    /* Delete if similar config already exists */
    /* is_add paramter is used to identify whether the user want to add or
       remove syslog configuration

       When the user wants to add new configuration, is_add will be set to true
       It is in intern used to alter the behaviour of the delete configuration
       function.  When is_add is true the delete configuration will try to
       find any configuration in the OVSDB which matchs the same remote host.
       If found it will simply remove the same so that we can add the newly
       provided configuration

       When the user wants to remove existing configuration is_add will be
       false.  When is_add is false delete configuration will try to find the
       exact match in the OVSDB table which matches all the parameters provided
       by the user.  If found it will remove the same, if such configuration is
       not found then it will print error message in vty_out and returns the
       error.
    */
    returncode = cli_syslog_delete_config(remote_host,transport,
            port_number,severity,
            system_row,is_add,!is_add);
    if(returncode != CMD_SUCCESS)
    {
        VLOG_ERR("Delete syslog config failed");
        cli_do_config_abort(txn);
        return returncode;
    }
    /* Following code is used only for adding new configuration */
    if(is_add)
    {
        /* Check the number of syslog remotes already configured.
           We support only upto MAX_REMOTE_SERVERS configuration.
           */
        if(system_row->n_syslog_remotes >= MAX_REMOTE_SERVERS)
        {
            vty_out(vty, "Limit of %d remote syslog servers already reached%s",
                     MAX_REMOTE_SERVERS, VTY_NEWLINE);
            cli_do_config_abort(txn);
            return CMD_WARNING;
        }

        /* Insert new row in the syslog_remote table */
        row = ovsrec_syslog_remote_insert(txn);

        if(row == NULL)
        {
            VLOG_ERR_SYSLOG_INSERT_FAILED;
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        /* Update remote host*/
        ovsrec_syslog_remote_verify_remote_host(row);
        ovsrec_syslog_remote_set_remote_host(row, remote_host);

        /* Update transport protocol if provided */
        if(transport != NULL)
        {
            ovsrec_syslog_remote_verify_transport(row);
            ovsrec_syslog_remote_set_transport(row, transport);
        }
        /* Update port number if provided */
        if(port_number != NULL && *port_number >= MIN_PORT_NUMBER_SUPPORTED &&
                *port_number <= MAX_PORT_NUMBER_SUPPORTED)
        {
            ovsrec_syslog_remote_verify_port_number(row);
            ovsrec_syslog_remote_set_port_number(row, port_number,1);
        }
        /* Update severity if provided */
        if(severity != NULL)
        {
            ovsrec_syslog_remote_verify_severity(row);
            ovsrec_syslog_remote_set_severity(row, severity);
        }
        /* Update the system table */
        row_array = (struct ovsrec_syslog_remote **)
            calloc(sizeof(struct ovsrec_syslog_remote *),
                    system_row->n_syslog_remotes+1);
        if(row_array == NULL)
        {
            VLOG_ERR_SYSLOG_INSERT_FAILED;
            cli_do_config_abort(txn);
            return CMD_OVSDB_FAILURE;
        }

        for(i = 0; i < system_row->n_syslog_remotes;i++)
        {
            row_array[i] = system_row->syslog_remotes[i];
        }
        row_array[i] = row;
        ovsrec_system_verify_syslog_remotes(system_row);
        ovsrec_system_set_syslog_remotes(system_row,
                row_array,
                system_row->n_syslog_remotes+1);
    }
    txn_status = cli_do_config_finish(txn);
    if(row_array)
    {
        free(row_array);
        row_array= NULL;
    }
    if(txn_status != TXN_SUCCESS && txn_status != TXN_UNCHANGED)
    {
        VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
        vty_out(vty,"Configuration failed%s",VTY_NEWLINE);
        return CMD_OVSDB_FAILURE;
    }
    return CMD_SUCCESS;

}

DEFUN (vtysh_config_syslog_basic,
       vtysh_config_syslog_basic_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       )
{
    return cli_syslog_config(argv[0],NULL,NULL,NULL,DO_LOGGING);
}

DEFUN (vtysh_config_syslog_udp,
       vtysh_config_syslog_udp_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) udp [<1-65535>]",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       UDP_PORT_STR
       )
{
    const int64_t* port_number = NULL;
    int64_t port;
    if(argc == 2)
    {
        /* If optional port_number is provided */
        port =(int64_t) atoi(argv[1]);
        port_number = &port;
    }
    return cli_syslog_config(argv[0],"udp",port_number,NULL,DO_LOGGING);
}


DEFUN (vtysh_config_syslog_tcp,
       vtysh_config_syslog_tcp_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) tcp [<1-65535>]",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       TCP_STR
       TCP_PORT_STR
       )
{
    const int64_t* port_number = NULL;
    int64_t port;
    if(argc == 2)
    {
        /* If optional port_number is provided */
        port =(int64_t) atoi(argv[1]);
        port_number = &port;
    }
    return cli_syslog_config(argv[0],"tcp",port_number,NULL,DO_LOGGING);
}

DEFUN (vtysh_config_syslog_svrt,
       vtysh_config_syslog_svrt_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) severity"
       " (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
       )
{
    return cli_syslog_config(argv[0],NULL,NULL,argv[1],DO_LOGGING);
}

DEFUN (vtysh_config_syslog_udp_svrt,
       vtysh_config_syslog_udp_svrt_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) udp [<1-65535>]"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       UDP_PORT_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    const int64_t* port_number = NULL;
    int64_t port;
    port = (int64_t)atoi(argv[1]);
    port_number = &port;

    return cli_syslog_config(argv[0],"udp",port_number,argv[2],DO_LOGGING);
}

DEFUN (vtysh_config_syslog_tcp_svrt,
       vtysh_config_syslog_tcp_svrt_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) tcp [<1-65535>]"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       TCP_STR
       TCP_PORT_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    const int64_t* port_number = NULL;
    int64_t port;
    port = (int64_t)atoi(argv[1]);
    port_number = &port;

    return cli_syslog_config(argv[0],"tcp",port_number,argv[2],DO_LOGGING);
}

DEFUN (vtysh_config_syslog_prot_svrt_noport,
       vtysh_config_syslog_prot_svrt_noport_cmd,
       "logging (A.B.C.D | X:X::X:X | WORD) (udp|tcp)"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       TCP_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    return cli_syslog_config(argv[0],argv[1],NULL,argv[2],DO_LOGGING);
}

/* No part of the commands */

DEFUN (no_vtysh_config_syslog_basic,
       no_vtysh_config_syslog_basic_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       )
{
    return cli_syslog_config(argv[0],NULL,NULL,NULL,0);
}

DEFUN (no_vtysh_config_syslog_udp,
       no_vtysh_config_syslog_udp_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD) udp [<1-65535>]",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       UDP_PORT_STR
       )
{
    const int64_t* port_number = NULL;
    int64_t port;
    if(argc == 2)
    {
        /* If optional port_number is provided */
        port =(int64_t) atoi(argv[1]);
        port_number = &port;
    }
    return cli_syslog_config(argv[0],"udp",port_number,NULL,DO_NO_LOGGING);
}


DEFUN (no_vtysh_config_syslog_tcp,
       no_vtysh_config_syslog_tcp_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD) tcp [<1-65535>]",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       TCP_STR
       TCP_PORT_STR
       )
{
    const int64_t* port_number = NULL;
    int64_t port;
    if(argc == 2)
    {
        /* If optional port_number is provided */
        port =(int64_t) atoi(argv[1]);
        port_number = &port;
    }
    return cli_syslog_config(argv[0],"tcp",port_number,NULL,DO_NO_LOGGING);
}

DEFUN (no_vtysh_config_syslog_svrt,
       no_vtysh_config_syslog_svrt_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD)"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
       )
{
    return cli_syslog_config(argv[0],NULL,NULL,argv[1],DO_NO_LOGGING);
}

DEFUN (no_vtysh_config_syslog_udp_svrt,
       no_vtysh_config_syslog_udp_svrt_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD) udp [<1-65535>]"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       UDP_PORT_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    const int64_t* port_number = NULL;
    int64_t port;
    port = (int64_t)atoi(argv[1]);
    port_number = &port;

    return cli_syslog_config(argv[0],"udp",port_number,argv[2],DO_NO_LOGGING);
}

DEFUN (no_vtysh_config_syslog_tcp_svrt,
       no_vtysh_config_syslog_tcp_svrt_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD) tcp [<1-65535>]"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       TCP_STR
       TCP_PORT_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    const int64_t* port_number = NULL;
    int64_t port;
    port = (int64_t)atoi(argv[1]);
    port_number = &port;

    return cli_syslog_config(argv[0],"tcp",port_number,argv[2],DO_NO_LOGGING);
}

DEFUN (no_vtysh_config_syslog_prot_svrt_noport,
       no_vtysh_config_syslog_prot_svrt_noport_cmd,
       "no logging (A.B.C.D | X:X::X:X | WORD) (udp|tcp)"
       " severity (debug|info|notice|warning|err|crit|alert|emerg)",
       LOGGING_STR
       HOST_IPv4
       HOST_IPv6
       HOSTNAME_STR
       UDP_STR
       TCP_STR
       SEVERITY_STR
       DEBUG_SVR_STR
       INFO_STR
       NOTICE_STR
       WARNING_STR
       ERR_STR
       CRIT_STR
       ALERT_STR
       EMERG_STR
     )
{
    return cli_syslog_config(argv[0],argv[1],NULL,argv[2],DO_NO_LOGGING);
}


DEFUN (vtysh_config_no_syslog,
       vtysh_config_no_syslog_cmd,
       "no logging",
       NO_STR
       LOGGING_STR
       )
{
    char flag = '0';
    static struct termios oldt, newt;
    const struct ovsrec_syslog_remote* row = NULL;
    enum ovsdb_idl_txn_status txn_status;
    struct ovsdb_idl_txn *txn = NULL;
    int retval = CMD_SUCCESS;
    const struct ovsrec_system *system_row = NULL;
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    vty_out(vty,"\rAll syslog server address configuration settings will be "\
            "removed.\nDo you want to continue [y/n]?");
    while(1)
    {
        flag=getchar();
        if (flag == 'y')
        {
            txn = cli_do_config_start();

            if(txn == NULL)
            {
                VLOG_ERR_SYSLOG_TRANSACTION_CREATE_FAILED;
                cli_do_config_abort(txn);
                retval = CMD_OVSDB_FAILURE;
                goto CLEAN_UP;
            }
            system_row = ovsrec_system_first(idl);
            if(system_row == NULL)
            {
                VLOG_ERR_SYSLOG_OPENVSWITCH_READ_FAILED;
                cli_do_config_abort(txn);
                retval =  CMD_OVSDB_FAILURE;
                goto CLEAN_UP;
            }


            vty_out(vty,"%s",VTY_NEWLINE);
            /* Delete all configuration */
            OVSREC_SYSLOG_REMOTE_FOR_EACH(row,idl)
            {
                ovsrec_syslog_remote_delete(row);
            }
            ovsrec_system_verify_syslog_remotes(system_row);
            ovsrec_system_set_syslog_remotes(system_row,
                    NULL,
                    0);
            txn_status = cli_do_config_finish(txn);
            if(txn_status != TXN_SUCCESS && txn_status != TXN_UNCHANGED)
            {
                VLOG_ERR(OVSDB_TXN_COMMIT_ERROR);
                vty_out(vty,"Configuration failed%s",VTY_NEWLINE);
                retval = CMD_OVSDB_FAILURE;
                goto CLEAN_UP;
            }
            break;
        }
        else if (flag == 'n')
        {
            vty_out(vty,"%s",VTY_NEWLINE);
            break;
        }
        else
        {
            vty_out(vty,"\r                                 ");
            vty_out(vty,"\rDo you want to continue [y/n]?");
        }
    }

CLEAN_UP:
    /*restore the old settings*/
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    return retval;
}



void
syslog_ovsdb_init(void)
{
    ovsdb_idl_add_table (idl, &ovsrec_table_syslog_remote);
    ovsdb_idl_add_column(idl, &ovsrec_syslog_remote_col_remote_host);
    ovsdb_idl_add_column(idl, &ovsrec_syslog_remote_col_severity);
    ovsdb_idl_add_column(idl, &ovsrec_syslog_remote_col_transport);
    ovsdb_idl_add_column(idl, &ovsrec_syslog_remote_col_port_number);
    ovsdb_idl_add_table(idl, &ovsrec_table_system);
    ovsdb_idl_add_column(idl, &ovsrec_system_col_syslog_remotes);
    return ;
}
