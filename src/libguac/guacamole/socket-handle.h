/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef GUAC_SOCKET_HANDLE_H
#define GUAC_SOCKET_HANDLE_H

/**
 * Provides an implementation of guac_socket specific to the Windows file handle
 * API. This header will (TODO) only be available if libguac was built with
 * cgywin etc etc.
 *
 * @file socket-handle.h
 */

#include "socket-types.h"

#include <handleapi.h>

/**
 * Creates a new guac_socket which will use the Windows handle API for all
 * communication. Freeing this guac_socket will automatically close the 
 * associated handle.
 *
 * @param handle
 *     The handle to use for the communicating with the connection underlying
 *     the created guac_socket.
 *
 * @return
 *     A newly-allocated guac_socket which will transparently use the Windows
 *     handle API for all communication.
 */
guac_socket* guac_socket_open_handle(HANDLE handle);

#endif
