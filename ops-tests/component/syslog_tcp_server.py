#
# Copyright (C) 2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# 'AS IS' BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.


import socket
import sys
from os import fsync

# Usage : python syslog_tcp_server.py <IP-ADDRESS> <PORT_NUMBER>
if len(sys.argv) >= 3:
    TCP_IP = sys.argv[1]
    TCP_PORT = int(sys.argv[2])
elif len(sys.argv) == 2:
    TCP_IP = sys.argv[1]
    TCP_PORT = 1470
else:
    exit()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind((TCP_IP, TCP_PORT))
sock.listen(5)
print('listening on %s:%s' % (TCP_IP, TCP_PORT))
while True:
    conn, addr = sock.accept()
    print('New Connection Established')
    try:
        f = open('/tmp/syslog_out.sb', 'w')
        while True:
            data = conn.recv(1024)
            if data:
                f.write(data)
                f.flush()
                fsync(f)
            else:
                break
    finally:
        print('Connection Closed')
        conn.close()
        f.close()
