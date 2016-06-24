/*****************************************************************************
 * time.c : time releated functions for NaCl.
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

#include <time.h>

unsigned vlc_clock_prec;
pthread_once_t vlc_clock_once = PTHREAD_ONCE_INIT;
void vlc_clock_setup_once(void) {
  struct timespec res;
  if(unlikely(clock_getres(CLOCK_MONOTONIC, &res) != 0 || res.tv_sec != 0))
    abort();
  vlc_clock_prec = (res.tv_nsec + 500) / 1000;
}

mtime_t mdate() {
  struct timespec ts;

  clock_setup();
  if (unlikely(clock_gettime(CLOCK_MONOTONIC, &ts) != 0))
    abort();

  return (INT64_C(1000000) * ts.tv_sec) + (ts.tv_nsec / 1000);
}

#undef mwait
void mwait(mtime_t deadline) {
  clock_setup();
  /* If the deadline is already elapsed, or within the clock precision,
   * do not even bother the system timer. */
  deadline -= vlc_clock_prec;

  struct timespec ts = mtime_to_ts(deadline);

  struct timespec now;
  long int nsec;
  long int sec;

  /* Get the current time for this clock.  */
  if(likely(clock_gettime(CLOCK_MONOTONIC, &now) != 0)) {
    fprintf(stderr, "clock_gettime failed\n");
    abort();
  }


  /* Compute the difference.  */
  nsec = ts.tv_nsec - now.tv_nsec;
  sec = ts.tv_sec - now.tv_sec - (nsec < 0);
  if(sec < 0)
    /* The time has already elapsed.  */
    return;

  now.tv_sec = sec;
  now.tv_nsec = nsec + (nsec < 0 ? 1000000000 : 0);

  while(nanosleep(&now, NULL) == EINTR);
}

#undef msleep
void msleep(mtime_t delay) {
    struct timespec ts = mtime_to_ts(delay);
    while (nanosleep (&ts, &ts) == -1) {
        assert (errno == EINTR);
    }
}
