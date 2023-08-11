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
 * Aquire the write lock for the provided read-write lock, if the provided key
 * does not indicate that the write-lock is already acquired. If the key
 * indicates that the read lock is already aquired, the read lock will be
 * dropped before the write lock is acquired.
 *
 * @param lock
 *     The read-write lock for which the write lock should be aquired, if
 *     necessary.
 *
 * @param key
 *     The key associated with the thread-local flag indicating which locks
 *     are already acquired by the current thread.
 */
void guac_acquire_write_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key);

/**
 * Aquire the read lock for the provided read-write lock, if the provided key
 * does not indicate that either the read or write locks are already acquired.
 *
 * @param lock
 *     The read-write lock for which the read lock should be aquired, if
 *     necessary.
 *
 * @param key
 *     The key associated with the thread-local flag indicating which locks
 *     are already acquired by the current thread.
 */
void guac_acquire_read_lock_if_needed(pthread_rwlock_t* lock, pthread_key_t key);

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
void guac_release_lock(pthread_rwlock_t* lock, pthread_key_t key);

#endif

