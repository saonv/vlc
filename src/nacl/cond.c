/*****************************************************************************
 * cond.c : pthread condition variable.
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

void vlc_cond_init(vlc_cond_t* condvar) {
  pthread_condattr_t attr;

  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

  int val = pthread_cond_init(condvar, &attr);
  VLC_THREAD_ASSERT("condition variable init");
}

void vlc_cond_init_daytime(vlc_cond_t* condvar) {
  pthread_condattr_t attr;

  pthread_condattr_init(&attr);
  pthread_condattr_setclock(&attr, CLOCK_REALTIME);

  int val = pthread_cond_init(condvar, &attr);
  VLC_THREAD_ASSERT("condition variable init");
}

void vlc_cond_destroy(vlc_cond_t* condvar) {
  int val = pthread_cond_destroy(condvar);
  VLC_THREAD_ASSERT("destroying condition");
}

void vlc_cond_signal(vlc_cond_t* condvar) {
  int val = pthread_cond_signal(condvar);
  VLC_THREAD_ASSERT("signaling condition variable");
}

void vlc_cond_broadcast(vlc_cond_t* condvar) {
  pthread_cond_broadcast(condvar);
}

void vlc_cond_wait(vlc_cond_t* condvar, vlc_mutex_t* mutex) {
  struct vlc_thread* t = __local_thread;

  if(t != NULL) {
    vlc_testcancel();
    if (vlc_mutex_trylock(&t->waiting_lock) == 0) {
      t->waiting = condvar;
      vlc_mutex_unlock(&t->waiting_lock);
    } else {
      vlc_testcancel();
      t = NULL;
    }
  }

  int val = pthread_cond_wait(condvar, mutex);
  VLC_THREAD_ASSERT("waiting on condition");

  if(t != NULL){
    vlc_mutex_lock(&t->waiting_lock);
    t->waiting = NULL;
    vlc_mutex_unlock(&t->waiting_lock);
    vlc_testcancel();
  }
}

int vlc_cond_timedwait(vlc_cond_t* condvar, vlc_mutex_t* mutex, mtime_t deadline) {
  clock_setup();
  /* If the deadline is already elapsed, or within the clock precision,
   * do not even bother the system timer. */
  deadline -= vlc_clock_prec;
  deadline -= mdate();

  /* deadline is now relative */

  if(deadline < 0) {
    return ETIMEDOUT;
  }

  /* Create an absolute time from deadline and CLOCK_REALTIME */
  struct timespec rt;
  if(unlikely(clock_gettime(CLOCK_REALTIME, &rt) != 0))
    abort();

  deadline += (INT64_C(1000000) * rt.tv_sec) + (rt.tv_nsec / 1000);

  struct timespec ts = mtime_to_ts(deadline);

  struct vlc_thread* t = __local_thread;

  if(t != NULL) {
    vlc_testcancel();
    if(vlc_mutex_trylock(&t->waiting_lock) == 0) {
      t->waiting = condvar;
      vlc_mutex_unlock(&t->waiting_lock);
    } else {
      vlc_testcancel();
      t = NULL;
    }
  }

  int val = pthread_cond_timedwait_abs(condvar, mutex, &ts);

  if(val != ETIMEDOUT)
    VLC_THREAD_ASSERT ("timed-waiting on condition");

  if(t != NULL) {
    vlc_mutex_lock(&t->waiting_lock);
    t->waiting = NULL;
    vlc_mutex_unlock(&t->waiting_lock);
    vlc_testcancel();
  }
  return val;
}
