/*****************************************************************************
 * cancel.c : pthread cancel.
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

// cancellation:

void vlc_cancel(vlc_thread_t id) {
  atomic_store(&id->killed, true);

  vlc_mutex_lock(&id->waiting_lock);
  if(id->waiting) {
    pthread_cond_broadcast(id->waiting);
  }
  vlc_mutex_unlock(&id->waiting_lock);
}

int vlc_savecancel(void) {
  struct vlc_thread *t = __local_thread;
  if (t == NULL)
    return false; // external thread

  int state = t->killable;
  t->killable = false;
  return state;
}

void vlc_restorecancel(int state) {
  struct vlc_thread *t = __local_thread;
  assert(state == false || state == true);

  if (t == NULL)
    return; // external

  assert(!t->killable);
  t->killable = state != 0;
}

void vlc_testcancel(void) {
  struct vlc_thread* local = __local_thread;
  if(!local) {
    return;
  }
  if(!local->killable) {
    return;
  }
  if(!atomic_load(&local->killed)) {
    return;
  }

  pthread_exit(NULL);
}

void vlc_control_cancel(int cmd, ...) {
  va_list ap;

  struct vlc_thread *t = __local_thread;
  if(t == NULL)
    return; // external

  va_start(ap, cmd);
  switch (cmd) {
  case VLC_CLEANUP_PUSH: {
    vlc_cleanup_t *cleaner = va_arg(ap, vlc_cleanup_t*);
    cleaner->next = t->cleaners;
    t->cleaners = cleaner;
    break;
  }

  case VLC_CLEANUP_POP: {
    assert(t->cleaners);
    t->cleaners = t->cleaners->next;
    break;
  }
  }
  va_end (ap);
}
