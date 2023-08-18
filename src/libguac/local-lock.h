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

#ifndef __GUAC_LOCAL_LOCK_H
#define __GUAC_LOCAL_LOCK_H

#include <pthread.h>

/**
 * This file implements reentrant read-write locks using thread-local storage
 * to keep track of how locks are held and released by the current thread,
 * since the pthread locks do not support reentrant behavior.
 *
 * A thread will attempt to acquire the requested lock on the first acquire
 * function call, and will release it once the number of unlock requests
 * matches the number of lock requests. Therefore, it is safe to aquire a lock
 * and then call a function that also acquires the same lock, provided that
 * the caller and the callee request to unlock the lock when done with it.
 *
 * Any lock that's locked using one of the functions defined in this file
 * must _only_ be unlocked using the unlock function defined here to avoid
 * unexpected behavior.
 */

/**
 * A structure packaging together a pthread rwlock along with a key to a
 * thread-local property to keep track of the current status of the lock,
 * allowing the functions defined in this header to provide reentrant behavior.
 * Note that both the lock and key must be initialized before being provided
 * to any of these functions.
 */
typedef struct guac_local_lock {

    /**
     * A non-reentrant pthread rwlock to be wrapped by the local lock,
     * functions providing reentrant behavior.
     */
    pthread_rwlock_t lock;

    /**
     * A key to access a thread-local property tracking any ownership of the
     * lock by the current thread.
     */
    pthread_key_t key;

} guac_local_lock;

/**
 * Aquire the write lock for the provided guac local lock, if the key does not
 * indicate that the write lock is already acquired. If the key indicates that
 * the read lock is already acquired, the read lock will be dropped before the
 * write lock is acquired. The thread local property associated with the key
 * will be updated as necessary to track the thread's ownership of the lock.
 *
 * @param local_lock
 *     The guac local lock for which the write lock should be acquired
 *     reentrantly.
 */
void guac_acquire_write_lock(guac_local_lock* local_lock);

/**
 * Aquire the read lock for the provided guac local lock, if the key does not
 * indicate that the read or write lock is already acquired. The thread local
 * property associated with the key will be updated as necessary to track the
 * thread's ownership of the lock.
 *
 * @param local_lock
 *     The guac local lock for which the read lock should be acquired
 *     reentrantly.
 */
void guac_acquire_read_lock(guac_local_lock* local_lock);

/**
 * Release the the rwlock associated with the provided guac local lock if this
 * is the last level of the lock held by this thread. Otherwise, the thread
 * local property associated with the key will be updated as needed to ensure
 * that the correct number of release requests will finally release the lock.
 *
 * @param local_lock
 *     The guac local lock that should be released.
 */
void guac_release_lock(guac_local_lock* local_lock);

#endif

