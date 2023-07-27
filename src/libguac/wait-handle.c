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

#include <stdio.h>

#include <errhandlingapi.h>
#include <windef.h>
#include <handleapi.h>
#include <winbase.h>

int guac_wait_for_handle(HANDLE handle, int usec_timeout) {

    HANDLE event = CreateEvent(

        /* YOLO: default security settings */
        NULL,

        /* Disable manual reset */
        FALSE,

        /* Initialize to not signalled so we can wait on it */
        FALSE,

        /* No name should be needed here */
        NULL

    );

    /* 
     * An overlapped structure, required for IO with any handle that's opened
     * in overlapped mode, with all fields initialized to zero to avoid errors.
     */
    OVERLAPPED overlapped = { 0 };
    
    /* Set the event to be used to signal comm events */
    overlapped.hEvent = event;

    /* Request to wait for new data to be available */
    char buff[1]; 
    if (!ReadFile(handle, &buff, 0, NULL, &overlapped)) {
        
        DWORD error = GetLastError();

        /* ERROR_IO_PENDING is expected in overlapped mode */
        if (error != ERROR_IO_PENDING) {
            return -1;
        }
    }

    int millis = (usec_timeout + 999) / 1000;
    //fprintf(stderr, "Waiting for %i milliseconds...\n", millis);
    
    DWORD result = WaitForSingleObject(event, millis);
    //fprintf(stderr, "The WaitForSingleObject result is %u\n", result);
    
    /* The wait attempt failed */ 
    if (result == WAIT_FAILED) {
        fprintf(stderr, "The WaitForSingleObject error was %i\n", GetLastError());
        return -1;
    }

    /* The event was signalled, which should indicate data is ready */
    else if (result == WAIT_OBJECT_0)
        return 1;

    /* 
     * If the event didn't trigger and the wait didn't fail, data just isn't 
     * ready yet.
     */
    return 0;
    
}
