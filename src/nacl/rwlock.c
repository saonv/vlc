/*****************************************************************************
 * thread.c : pthread create.
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

void vlc_rwlock_init(vlc_rwlock_t* lock) {
  const int val = pthread_rwlock_init(lock, NULL);
  VLC_THREAD_ASSERT("initializing R/W lock");
}

void vlc_rwlock_destroy(vlc_rwlock_t* lock) {
  const int val = pthread_rwlock_destroy(lock);
  VLC_THREAD_ASSERT("destroying R/W lock");
}

void vlc_rwlock_rdlock(vlc_rwlock_t* lock) {
  const int val = pthread_rwlock_rdlock(lock);
  VLC_THREAD_ASSERT("acquiring R/W lock for reading");
}

void vlc_rwlock_wrlock(vlc_rwlock_t* lock) {
  const int val = pthread_rwlock_wrlock(lock);
  VLC_THREAD_ASSERT("acquiring R/W lock for writing");
}

void vlc_rwlock_unlock(vlc_rwlock_t* lock) {
  const int val = pthread_rwlock_unlock(lock);
  VLC_THREAD_ASSERT("releasing R/W lock");
}
