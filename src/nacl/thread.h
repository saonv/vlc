/*****************************************************************************
 * thread.c : NaCl-specific back-end for pthreads
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_atomic.h>

#include "libvlc.h"
#include <errno.h>
#include <assert.h>

/// Based off Android's emulation layer.

struct vlc_thread {
  pthread_t       id;
  pthread_cond_t* waiting;
  vlc_mutex_t     waiting_lock;
  vlc_sem_t       finished;
  int             schedprio;

  void* (*entry)(void*);
  void* entry_data;

  atomic_bool killed;
  bool killable;

  vlc_cleanup_t* cleaners;
};

// Note: NaCl thread local reads call a function. So reads of the global should
// be to a stack local variable.
static __thread struct vlc_thread* __local_thread = NULL;

#ifndef NDEBUG
/**
 * Reports a fatal error from the threading layer, for debugging purposes.
 */
static inline void vlc_thread_fatal(const char* action, const int error,
                                    const char* function, const char* file,
                                    const unsigned line) {
  VLC_UNUSED(function); VLC_UNUSED(file); VLC_UNUSED(line);

  int canc = vlc_savecancel();
  fprintf(stderr, "LibVLC fatal error %s (%d) in thread %u ",
          action, error, (uintptr_t)(pthread_self()));
  perror("Thread error");
  fflush(stderr);

  vlc_restorecancel(canc);
  abort();
}

# define VLC_THREAD_ASSERT(action) \
    if(unlikely(val)) \
        vlc_thread_fatal(action, val, __func__, __FILE__, __LINE__)
#else
# define VLC_THREAD_ASSERT(action) ((void)val)
#endif

static inline struct timespec mtime_to_ts(const mtime_t date) {
  const lldiv_t d = lldiv(date, CLOCK_FREQ);
  const struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };

  return ts;
}

extern unsigned vlc_clock_prec;
void vlc_clock_setup_once(void);

extern pthread_once_t vlc_clock_once;
#define clock_setup() pthread_once(&vlc_clock_once, vlc_clock_setup_once)
