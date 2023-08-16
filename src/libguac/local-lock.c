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

#include <pthread.h>
#include "local-lock.h"

/**
 * The value indicating that the current thread holds neither the read or write
 * locks.
 */
#define GUAC_LOCAL_LOCK_NO_LOCK 0

/**
 * The value indicating that the current thread holds the read lock.
 */
#define GUAC_LOCAL_LOCK_READ_LOCK 1

/**
 * The value indicating that the current thread holds the write lock.
 */
#define GUAC_LOCAL_LOCK_WRITE_LOCK 2

/**
 * Extract and return the flag indicating which lock is held, if any, from the
 * provided key value. The flag is always stored in the least-significant
 * nibble of the value.
 *
 * @param value
 *     The key value containing the flag.
 *
 * @return
 *     The flag indicating which lock is held, if any.
 */
static __intptr_t get_lock_flag(__intptr_t value) {
    return value & 0xF;
}

/**
 * Extract and return the lock count from the provided key. This returned value
 * is the difference between the number of lock and unlock requests made by the
 * current thread. This count is always stored in the remaining value after the
 * least-significant nibble where the flag is stored.
 *
 * @param value
 *     The key value containing the count.
 *
 * @return
 *     The difference between the number of lock and unlock requests made by
 *     the current thread.
 */
static __intptr_t get_lock_count(__intptr_t value) {
    return value >> 4;
}

/**
 * Given a flag indicating if and how the current thread controls a lock, and
 * a count of the depth of lock requests, return a value containing the flag
 * in the least-significant nibble, and the count in the rest.
 *
 * @param flag
 *     A flag indiciating which lock, if any, is held by the current thread.
 *
 * @param count
 *     The depth of the lock attempt by the current thread, i.e. the number of
 *     lock requests minus unlock requests.
 *
 * @return
 *     A value containing the flag in the least-significant nibble, and the
 *     count in the rest, cast to a void* for thread-local storage.
 */
static void* get_value_from_flag_and_count(
        __intptr_t flag, __intptr_t count) {
    return (void*) ((flag & 0xF) | count << 4);
}

void guac_acquire_write_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key) {

    __intptr_t key_value = (__intptr_t) pthread_getspecific(key);
    __intptr_t flag = get_lock_flag(key_value);
    __intptr_t count = get_lock_count(key_value);

    /* If the current thread already holds the write lock, increment the count */
    if (flag == GUAC_LOCAL_LOCK_WRITE_LOCK) {
        pthread_setspecific(key, get_value_from_flag_and_count(
                flag, count + 1));

        /* This thread already has the lock */
        return;
    }

    /*
     * The read lock must be released before the write lock can be acquired.
     * This is a little odd because it may mean that a function further down
     * the stack may have requested a read lock, which will get upgraded to a
     * write lock by another function without the caller knowing about it. This
     * shouldn't cause any issues, however.
     */
    if (key_value == GUAC_LOCAL_LOCK_READ_LOCK)
        pthread_rwlock_unlock(lock);

    /* Acquire the write lock */
    pthread_rwlock_wrlock(lock);

    /* Mark that the current thread has the lock, and increment the count */
    pthread_setspecific(key, get_value_from_flag_and_count(
            GUAC_LOCAL_LOCK_WRITE_LOCK, count + 1));

}

void guac_acquire_read_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key) {

    __intptr_t key_value = (__intptr_t) pthread_getspecific(key);
    __intptr_t flag = get_lock_flag(key_value);
    __intptr_t count = get_lock_count(key_value);

    /* The current thread may read if either the read or write lock is held */
    if (
            key_value == GUAC_LOCAL_LOCK_READ_LOCK ||
            key_value == GUAC_LOCAL_LOCK_WRITE_LOCK
    ) {

        /* Increment the depth counter */
        pthread_setspecific(key, get_value_from_flag_and_count(
                flag, count + 1));

        /* This thread already has the lock */
        return;
    }

    /* Acquire the lock */
    pthread_rwlock_rdlock(lock);

    /* Set the flag that the current thread has the read lock */
    pthread_setspecific(key, get_value_from_flag_and_count(
                GUAC_LOCAL_LOCK_READ_LOCK, 1));

}

void guac_release_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key) {

    __intptr_t key_value = (__intptr_t) pthread_getspecific(key);
    __intptr_t flag = get_lock_flag(key_value);
    __intptr_t count = get_lock_count(key_value);

    /* Release the lock if this is the last locked level */
    if (count <= 1) {
        pthread_rwlock_unlock(lock);

        /* Set the flag that the current thread holds no locks */
        pthread_setspecific(key, get_value_from_flag_and_count(
                GUAC_LOCAL_LOCK_NO_LOCK, 0));
    }

    /* Do not release the lock since it's still in use - just decrement */
    pthread_setspecific(key, get_value_from_flag_and_count(
            flag, count - 1));

}
