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

#include "config.h"
#include "log.h"
#include "move-handle.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <guacamole/client-types.h>

/* Windows headers */
#include <errhandlingapi.h>
#include <fcntl.h>
#include <handleapi.h>
#include <io.h>
#include <processthreadsapi.h>
#include <windows.h>
#include <sys/cygwin.h>

int guacd_send_handle(int pid, int sock, HANDLE handle) {

    /* Handle to be duplicated into the target process */
    HANDLE target_handle;

    /* Convert the unix PID from fork() to a windows pid */
    uintptr_t win_pid = cygwin_internal(CW_CYGWIN_PID_TO_WINPID, pid);

    /* 
     * Create handle for the target process. NOTE: windows uses handles for
     * many different things. This particular handle refers to another process,
     * not a file.
     */
    HANDLE process_handle = OpenProcess(PROCESS_DUP_HANDLE, 0, win_pid);
    if (process_handle == NULL) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to allocate handle for process ID %d: %d", 
                win_pid, GetLastError());
        return 0;
    }

    /* Duplicate the file description into the target process */
    BOOL handle_created = DuplicateHandle(
            GetCurrentProcess(), 
            handle, 
            process_handle,
            &target_handle, 
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);
        
    CloseHandle(process_handle);
    CloseHandle(target_handle);

    if (!handle_created) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to duplicate handle: %d", GetLastError());
        return 0;
    }

    /* 
     * Split the handle into bytes to send across the handle.
     * NOTE: This does NOT convert to network byte order, and instead relies
     * on the target process, running on the same system, using the same byte
     * order. This should be fine...
     */
    struct msghdr message = {0};
    char message_data[sizeof(target_handle)];
    memcpy(&message_data, &target_handle, sizeof(target_handle));

    /* Assign data buffer */
    struct iovec io_vector[1];
    io_vector[0].iov_base = message_data;
    io_vector[0].iov_len  = sizeof(message_data);
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;

    guacd_log(GUAC_LOG_INFO, "Sending handle %p", target_handle);

    /* Send file descriptor */
    return (sendmsg(sock, &message, 0) == sizeof(message_data));

}

HANDLE guacd_recv_handle(int sock) {

    HANDLE handle = NULL;

    struct msghdr message = {0};
    char message_data[sizeof(HANDLE)];

    /* Assign data buffer */
    struct iovec io_vector[1];
    io_vector[0].iov_base = message_data;
    io_vector[0].iov_len  = sizeof(message_data);
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;

    /* Receive file descriptor */
    if (recvmsg(sock, &message, 0) == sizeof(message_data)) {
        
        /* 
         * Copy the data from the message to get the handle, which was
         * previously copied into this process using DuplicateHandle()
         */
        memcpy(&handle, &message_data, sizeof(HANDLE));

        guacd_log(GUAC_LOG_INFO, "Got handle %p", handle);

    } /* end if recvmsg() success */

    /* Return the handle, if any */
    return handle;

}

