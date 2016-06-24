/*****************************************************************************
 * sem.c : semaphores.
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

void vlc_sem_init(vlc_sem_t* sem, unsigned value) {
  if(likely(sem_init(sem, 0, value) == 0)) {
    return;
  }

  const int val = errno;
  VLC_THREAD_ASSERT("destroying semaphore");
}

void vlc_sem_destroy(vlc_sem_t* sem) {
  if(likely(sem_destroy(sem) == 0)) {
    return;
  }

  const int val = errno;
  VLC_THREAD_ASSERT("destroying semaphore");
}

int vlc_sem_post(vlc_sem_t* sem) {
  if(likely(sem_post(sem) == 0)) {
    return 0;
  }

  const int val = errno;

  if(unlikely(val != EOVERFLOW)) {
    VLC_THREAD_ASSERT("unlocking semaphore");
  }
  return val;
}

void vlc_sem_wait(vlc_sem_t* sem) {
  int val = 0;

  do {
    if(likely(sem_wait(sem) == 0)) {
      return;
    }
  } while ((val = errno) == EINTR);

  VLC_THREAD_ASSERT("locking semaphore");
}
