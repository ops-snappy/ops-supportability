/* Syslog client callback resigitration source files.
 *
 * Copyright (C) 2016 Hewlett Packard Enterprise Development LP.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * File: vtysh_ovsdb_syslog_context.c
 *
 * Purpose: Source for registering sub-context callback with
 *          global config context.
 */

#include "vtysh/vty.h"
#include "vtysh/vector.h"
#include "vswitch-idl.h"
#include "openswitch-idl.h"
#include "vtysh/vtysh_ovsdb_if.h"
#include "vtysh/vtysh_ovsdb_config.h"
#include "vtysh/utils/system_vtysh_utils.h"
#include "vtysh_ovsdb_syslog_context.h"


#define SEVERITYKEY     " severity "

/* Function         :   vtysh_config_context_syslog_clientcallback
 * Responsibility   :   call back function to display show running for
 *                      syslog configuration
 * Returns          :   e_vtysh_ok
 */
vtysh_ret_val
vtysh_config_context_syslog_clientcallback(void *p_private)
{
    vtysh_ovsdb_cbmsg_ptr p_msg = (vtysh_ovsdb_cbmsg *)p_private;
    const struct ovsrec_syslog_remote *row = NULL;
    const char* transport = "";
    const char* transkey = " ";
    const char* severity = "";
    const char* sevkey = SEVERITYKEY;
    vtysh_ovsdb_config_logmsg(VTYSH_OVSDB_CONFIG_DBG,
            "vtysh_config_context_syslog_clientcallback entered");
    OVSREC_SYSLOG_REMOTE_FOR_EACH(row,p_msg->idl)
    {
        if(row->transport == NULL)
        {
            transport = "";
            transkey = "";
        }
        else
        {
            transport = row->transport;
            transkey = " ";
        }
        if(row->severity == NULL)
        {
            severity = "";
            sevkey = "";
        }
        else
        {
            severity = row->severity;
            sevkey = SEVERITYKEY;
        }
        if( row->port_number == NULL)
        {
            vtysh_ovsdb_cli_print(p_msg,"logging %s%s%s%s%s"
                ,row->remote_host,transkey,transport,sevkey,severity);
        }
        else
        {
            vtysh_ovsdb_cli_print(p_msg,"logging %s%s%s %d%s%s"
                ,row->remote_host,transkey,transport
                ,(int)(*row->port_number)
                ,sevkey,severity);
        }
    }
    return e_vtysh_ok;
}
