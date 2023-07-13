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

#include <windef.h>
#include <handleapi.h>
#include <winbase.h>

int guac_wait_for_handle(HANDLE handle, int usec_timeout) {

    /* 
     * Only wait for received data.
     * TODO: Should this be set just once?
     */
    SetCommMask(handle, EV_RXCHAR);

    /* The originally configured timeouts for this handle */
    COMMTIMEOUTS original_timeouts;
    if (GetCommTimeouts(handle, &original_timeouts))
        return -1;

    /* New timeout configuration, specific to this wait */
    COMMTIMEOUTS wait_timeouts;

    /* Round up to the supported granularity */
    wait_timeouts.ReadTotalTimeoutConstant = (usec_timeout + 999) / 1000;
    
    /* Do not set any other timeout other than the overall read timeout */
    wait_timeouts.ReadIntervalTimeout = 0;
    wait_timeouts.ReadTotalTimeoutMultiplier = 0;
    wait_timeouts.WriteTotalTimeoutConstant = 0;
    wait_timeouts.WriteTotalTimeoutMultiplier = 0;

    // Set the configured timeout
    if (SetCommTimeouts(handle, &wait_timeouts))
        return -1;

    /* 
     * The type of event that occurred. This isn't actually used, since the
     * comm mask is already configured to only wait for EV_RXCHAR, but this
     * argument is not optional for WaitCommEvent().
     */
    DWORD event_mask;

    /* Wait for new data to be available */
    BOOL success = WaitCommEvent(handle, &event_mask, NULL);

    /* Restore the original timeout config. TODO: Is this actually needed? */
    SetCommTimeouts(handle, &original_timeouts);
        
    /* This MIGHT be the error that it returns if the wait times out... */
    if (!success && GetLastError() == WAIT_TIMEOUT)
        return 0;

    /* Otherwise, return 1 for sucess and -1 for error */
    return success ? 1 : -1;
    
}
