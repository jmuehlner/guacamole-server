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

void guac_acquire_write_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key) {

    __intptr_t key_value = (__intptr_t) pthread_getspecific(key);

    /* If the current thread already holds the write lock, nothing to do */
    if (key_value == GUAC_LOCAL_LOCK_WRITE_LOCK)
        return;

    /* The read lock must be released before the write lock can be acquired */
    if (key_value == GUAC_LOCAL_LOCK_READ_LOCK)
        guac_release_lock(lock, key);

    /* Acquire the lock */
    pthread_rwlock_wrlock(lock);

    /* Set the flag that the current thread has the write lock */
    pthread_setspecific(key, (void*) GUAC_LOCAL_LOCK_WRITE_LOCK);

}

void guac_acquire_read_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key) {

    __intptr_t key_value = (__intptr_t) pthread_getspecific(key);

    /* The current thread may read if either the read or write lock is held */
    if (
            key_value == GUAC_LOCAL_LOCK_READ_LOCK ||
            key_value == GUAC_LOCAL_LOCK_WRITE_LOCK
    )
        return;

    /* Acquire the lock */
    pthread_rwlock_rdlock(lock);

    /* Set the flag that the current thread has the write lock */
    pthread_setspecific(key, (void*) GUAC_LOCAL_LOCK_READ_LOCK);

}

/**
 * Release the the provided read-write lock, setting the provided key to
 * indicate that the lock has been released.
 *
 * @param lock
 *     The read-write lock that should be released.
 *
 * @param key
 *     The key associated with the thread-local flag indicating which locks
 *     are already acquired by the current thread.
 */
void guac_release_lock(pthread_rwlock_t* lock, pthread_key_t key) {

    /* Release the lock */
    pthread_rwlock_unlock(lock);

    /* Set the flag that the current thread holds no locks */
    pthread_setspecific(key, (void*) GUAC_LOCAL_LOCK_NO_LOCK);

}
