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
#include "move-fd.h"

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

int guacd_send_fd(int pid, int sock, int fd) {

    /* Create handle from file descriptor */
    HANDLE fd_handle = (HANDLE) _get_osfhandle(fd);
    if (fd_handle == NULL) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to allocate handle for file descriptor: %d", 
                GetLastError());
        return 0;
    }

    /* Handle duplicated into the target process */
    HANDLE target_handle;

    /* Create handle for the target process */
    HANDLE process_handle = OpenProcess(PROCESS_DUP_HANDLE, 0, pid);
    if (process_handle == NULL) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to allocate handle for process ID: %d", 
                GetLastError());
        CloseHandle(fd_handle);
        return 0;
    }

    /* Duplicate the file description into the target process */
    BOOL handle_created = DuplicateHandle(
            GetCurrentProcess(), 
            fd_handle, 
            process_handle,
            &target_handle, 
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);

    if (!handle_created) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to duplicate handle: %d", GetLastError());
        CloseHandle(fd_handle);
        CloseHandle(process_handle);
        CloseHandle(target_handle);
        return 0;
    }

    /* 
     * Split the handle into bytes to send across the handle 
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

    /* Send file descriptor */
    return (sendmsg(sock, &message, 0) == sizeof(message_data));

}

int guacd_recv_fd(int sock) {

    HANDLE fd_handle;

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
        memcpy(&fd_handle, &message, sizeof(HANDLE));

        /* Convert from a windows handle to a unix-style file descriptor */
        return _open_osfhandle(fd_handle, _O_TEXT);

    } /* end if recvmsg() success */

    /* Failed to receive file descriptor */
    return -1;

}

