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

#ifndef GUACD_MOVE_HANDLE_H
#define GUACD_MOVE_HANDLE_H

#include <handleapi.h>

/**
 * Sends the given file handles along the given socket, allowing the receiving
 * process to use the file handles normally. Returns non-zero on success, zero 
 * on error, just as a normal call to sendmsg() would. If an error does occur, 
 * GetLastError() will return the appropriate error.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the file
 *     handle specified by handle should be sent.
 *
 * @param pid
 *     The ID of the process to which the the file handle specified by handle 
 *     should be sent.
 *
 * @param write_handle
 *     The write file handle to send along the given UNIX domain socket.
 *
 * @param read_handle
 *     The read file handle to send along the given UNIX domain socket.
 *
 * @return
 *     Non-zero if the send operation succeeded, zero on error.
 */
int guacd_send_handles(int pid, int sock, HANDLE write_handle, HANDLE read_handle);

/**
 * Waits for a file handle on the given socket, populating the received file
 * handle. The file handle must have been sent via guacd_send_handle. If an
 * error occurs, -1 is returned, and errno will be set appropriately.
 *
 * @param sock
 *     The file descriptor of an open UNIX domain socket along which the file
 *     handle will be sent (by guacd_send_handle()).
 *
 * @param handles
 *     An array of HANDLEs to be populated.
 *
 * @return
 *     Non-zero if the receive operation succeeded, zero on error
 */
int guacd_recv_handles(int sock, HANDLE* handles);

#endif

