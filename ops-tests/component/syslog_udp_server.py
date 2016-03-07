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

# Usage : python syslog_udp_server.py <IP-ADDRESS> <PORT_NUMBER>
if len(sys.argv) >= 3:
    UDP_IP = sys.argv[1]
    UDP_PORT = int(sys.argv[2])
elif len(sys.argv) == 2:
    UDP_IP = sys.argv[1]
    UDP_PORT = 11514
else:
    exit()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

sock.bind((UDP_IP, UDP_PORT))

f = open('/tmp/syslog_out.sb', 'w')
i = 0
while i < 50:
    data, addr = sock.recvfrom(1024)
    f.write(data)
    f.flush()
    fsync(f)
    i = i + 1
f.close()
