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

void vlc_threads_setup(libvlc_int_t* libvlc) {
  VLC_UNUSED(libvlc);
}

// thread creation:

static void detached_thread_cleanup(void* data) {
  struct vlc_thread* thread = (struct vlc_thread*)data;
  vlc_mutex_destroy(&thread->waiting_lock);
  free(thread);
}
static void* detached_thread_entry(void* data) {
  struct vlc_thread* t = (struct vlc_thread*)data;
  __local_thread = t;

  vlc_cleanup_push(detached_thread_cleanup, t);

  // TODO: ignores errors.
  pthread_setschedprio(t->id, t->schedprio);

  t->entry(t->entry_data);
  vlc_cleanup_pop();
  detached_thread_cleanup(data);

  return NULL;
}
static void joinable_thread_cleanup(void* data) {
  struct vlc_thread* t = (struct vlc_thread*)data;
  vlc_sem_post(&t->finished);
}
static void* joinable_thread_entry(void* data) {
  struct vlc_thread* t = (struct vlc_thread*)data;
  __local_thread = t;

  void* ret = NULL;

  vlc_cleanup_push(joinable_thread_cleanup, data);

  // TODO: ignores errors.
  pthread_setschedprio(t->id, t->schedprio);

  ret = t->entry(t->entry_data);
  vlc_cleanup_pop();
  joinable_thread_cleanup(data);

  return ret;
}

static int vlc_clone_attr(vlc_thread_t* id, void* (*entry)(void*),
                          void* data, const bool detach,
                          const int priority) {
  struct vlc_thread* thread = (struct vlc_thread*)malloc(sizeof(struct vlc_thread));
  if(unlikely(thread == NULL)) {
    return ENOMEM;
  }

  if(!detach) {
    vlc_sem_init(&thread->finished, 0);
  }

  atomic_store(&thread->killed, false);
  thread->killable = true;
  thread->waiting = NULL;
  thread->entry = entry;
  thread->entry_data = data;
  thread->schedprio = priority;
  thread->cleaners = NULL;
  vlc_mutex_init(&thread->waiting_lock);

  int detached_state;
  if(detach) {
    entry = detached_thread_entry;
    detached_state = PTHREAD_CREATE_DETACHED;
  } else {
    entry = joinable_thread_entry;
    detached_state = PTHREAD_CREATE_JOINABLE;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, detached_state);

  int ret = pthread_create(&thread->id, &attr, entry, (void*)thread);
  pthread_attr_destroy(&attr);

  if(!ret && id != NULL) {
    *id = (struct vlc_thread*)thread;
  }

  if(ret) {
    if(id != NULL) {
      *id = NULL;
    }
    free(thread);
  }
  return ret;
}
int vlc_clone(vlc_thread_t* id, void* (*entry)(void*), void* data,
              int priority) {
  return vlc_clone_attr(id, entry, data, false, priority);
}
int vlc_clone_detach(vlc_thread_t* id, void* (*entry)(void*), void* data,
                     int priority) {
  return vlc_clone_attr(id, entry, data, true, priority);
}
void vlc_join(vlc_thread_t id, void** result) {
  vlc_sem_wait(&id->finished);
  vlc_sem_destroy(&id->finished);

  int val = pthread_join(id->id, result);
  VLC_THREAD_ASSERT("vlc_join");
  vlc_mutex_destroy(&id->waiting_lock);
  free(id);
}
int vlc_set_priority(vlc_thread_t id, int priority) {
  if(pthread_setschedprio(id->id, priority)) {
    return VLC_EGENERIC;
  } else {
    return VLC_SUCCESS;
  }
}

unsigned long vlc_thread_id (void)
{
     return -1;
}

unsigned vlc_GetCPUCount(void) {
  return sysconf(_SC_NPROCESSORS_ONLN);
}
