/*****************************************************************************
 * threadvar.c : pthread threadvars.
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

int vlc_threadvar_create(vlc_threadvar_t* key, void (*destr) (void*)) {
  return pthread_key_create (key, destr);
}

void vlc_threadvar_delete (vlc_threadvar_t* tls) {
  pthread_key_delete(*tls);
}

int vlc_threadvar_set(vlc_threadvar_t key, void* value) {
  return pthread_setspecific(key, value);
}

void* vlc_threadvar_get(vlc_threadvar_t key) {
  return pthread_getspecific(key);
}
