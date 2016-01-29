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

cmake_minimum_required (VERSION 2.8)

set (SUPPORTABILITY_LIBS supportability)
project (${SUPPORTABILITY_LIBS})
set (SRC_DIR src)
set (INCL_DIR include)

# Rules to locate needed libraries
include(FindPkgConfig)
pkg_check_modules(OVSCOMMON REQUIRED libovscommon)

# Source files to build ops-supportability library
set (SOURCES ${SRC_DIR}/eventlog/eventlog.c)
include_directories (${PROJECT_SOURCE_DIR}/${INCL_DIR} ${OVSCOMMON_INCLUDE_DIRS})

# Rules to build ops-supportability library
add_library (${SUPPORTABILITY_LIBS} SHARED ${SOURCES})

# Rules to build supportability cli library
add_subdirectory(src/cli)

target_link_libraries(${SUPPORTABILITY_LIBS} ${OVSCOMMON_LIBRARIES} -lyaml -lsystemd)

# Define compile flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")

set(OPS_U_VER_MAJOR "0")
set(OPS_U_VER_MINOR "1")
set(OPS_U_VER_PATCH "0")
set(OPS_SUPPORTABILITY_VERSION "${OPS_U_VER_MAJOR}.${OPS_U_VER_MINOR}.${OPS_U_VER_PATCH}")
set_target_properties(${SUPPORTABILITY_LIBS} PROPERTIES VERSION ${OPS_SUPPORTABILITY_VERSION})

configure_file(${SRC_DIR}/opssupportability.pc.in ${SRC_DIR}/opssupportability.pc @ONLY)

# Rules to stage ops-supportability library and header files
install(TARGETS ${SUPPORTABILITY_LIBS}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

install(FILES ${INCL_DIR}/eventlog.h   ${INCL_DIR}/diag_dump.h
        DESTINATION include)

install(FILES ${CMAKE_BINARY_DIR}/${SRC_DIR}/opssupportability.pc DESTINATION lib/pkgconfig)
