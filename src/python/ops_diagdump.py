#
# Copyright (C) 2016 Hewlett-Packard Development Company, L.P.
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#

import ovs.unixctl.server


_diag_dump_cb_global = None


# Function          : init_diag_dump_basic
# Responsibility    : Registers handler function for unixctl command
# Parameters        : diag-dump callback handler function
# Returns           : void
def init_diag_dump_basic(diag_daemon_cb):
    global _diag_dump_cb_global
    _diag_dump_cb_global = diag_daemon_cb
    min_arg = 2
    max_arg = 2
    #vtysh is client for this unixctl server
    #vtysh is sending two argument to this server
    ovs.unixctl.command_register("dumpdiagbasic", "", min_arg, max_arg,
                                 diag_basic_unxctl_cb, None)


# Function          : diag_basic_unxctl_cb
# Responsibility    : Callback handler function for unixctl command
#                     It invokes callback function provided by user during
#                     initilization and send unixctl reply
# Parameters        : conn   - unixctl socket connection
#                     argv   - list contains 1)basic  2)feature name
# Returns           : void
def diag_basic_unxctl_cb(conn, argv, unused_aux):
    # argv[0] is basic
    # argv[1] is feature name
    if _diag_dump_cb_global is None:
        err_desc = 'No handler registered for basic diagnostics dump'
        buff = ' Internal error : ' + err_desc
    else:
        try:
            buff = _diag_dump_cb_global(argv)
        except:
            buff = ' An exception occured during diagnostics dump '

    conn.reply(buff)
