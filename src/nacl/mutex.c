/*****************************************************************************
 * mutex.c : pthread mutexes
 *****************************************************************************
 * Copyright (C) 1999-2016 VLC authors and VideoLAN
 *
 * Authors: Jean-Marc Dressler <polux@via.ecp.fr>
 *          Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@netcourrier.com>
 *          Clément Sténac
 *          Rémi Denis-Courmont
 *          Pierre Ynard
 *          Richard Diamond
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "thread.h"

void vlc_mutex_init(vlc_mutex_t* mutex) {
    pthread_mutexattr_t attr;

    int val;
    val = pthread_mutexattr_init(&attr);
    VLC_THREAD_ASSERT("mutex attribute init");
#ifdef NDEBUG
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    val = pthread_mutex_init(mutex, &attr);
    VLC_THREAD_ASSERT("mutex init");
    pthread_mutexattr_destroy(&attr);
}

void vlc_mutex_init_recursive(vlc_mutex_t* mutex) {
    pthread_mutexattr_t attr;

    int val;
    val = pthread_mutexattr_init(&attr);
    VLC_THREAD_ASSERT("mutex attribute init");

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#ifdef NDEBUG
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif

    val = pthread_mutex_init(mutex, &attr);
    VLC_THREAD_ASSERT("mutex init");

    pthread_mutexattr_destroy(&attr);
}

void vlc_mutex_destroy(vlc_mutex_t* mutex) {
  int val = pthread_mutex_destroy(mutex);
  VLC_THREAD_ASSERT("destroying mutex");
}

void vlc_mutex_lock(vlc_mutex_t* mutex) {
  int val = pthread_mutex_lock(mutex);
  VLC_THREAD_ASSERT ("locking mutex");
}

int vlc_mutex_trylock(vlc_mutex_t* mutex) {
  int val = pthread_mutex_trylock(mutex);

  if(val != EBUSY)
    VLC_THREAD_ASSERT("locking mutex");
  return val;
}

void vlc_mutex_unlock(vlc_mutex_t* mutex) {
  int val = pthread_mutex_unlock(mutex);
  VLC_THREAD_ASSERT("unlocking mutex");
}
