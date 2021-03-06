/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2014 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Contributed by Markus Deuling <deuling@de.ibm.com>  */

#include <stdio.h>
#include <stdlib.h>
#include <libspe2.h>
#include <pthread.h>
#include <sys/wait.h>

extern spe_program_handle_t bt_spu;
extern spe_program_handle_t bt2_spu;
#define nr_t 5

void *
spe_thread (void * arg)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t *ctx = (spe_context_ptr_t *) arg;

  spe_program_load (*ctx, &bt_spu);
  spe_context_run (*ctx, &entry, flags, NULL, NULL, NULL);

  pthread_exit (NULL);
}

int
indirect_handler (unsigned char *base, unsigned long offset)
{
  int flags = 0;
  unsigned int entry = SPE_DEFAULT_ENTRY;
  spe_context_ptr_t ctx = spe_context_create (0, NULL);

  spe_program_load (ctx, &bt2_spu);
  spe_context_run (ctx, &entry, flags, NULL, NULL, NULL);

  return 0;
}

static pthread_mutex_t crash_mutex = PTHREAD_MUTEX_INITIALIZER;
static int crash_count = 0;

int
crash_handler (unsigned char *base, unsigned long offset)
{
  int count;

  pthread_mutex_lock (&crash_mutex);
  count = ++crash_count;
  pthread_mutex_unlock (&crash_mutex);

  while (count < nr_t)
    ;

  abort ();
}

int
main (void)
{
  int thread_id[nr_t];
  pthread_t pts[nr_t];
  spe_context_ptr_t ctx[nr_t];
  int value = 1;
  int cnt;

  spe_callback_handler_register (indirect_handler, 0x11, SPE_CALLBACK_NEW);
  spe_callback_handler_register (crash_handler, 0x12, SPE_CALLBACK_NEW);

  for (cnt = 0; cnt < nr_t; cnt++)
    {
      ctx[cnt] = spe_context_create (0, NULL);
      thread_id[cnt]
	= pthread_create (&pts[cnt], NULL, &spe_thread, &ctx[cnt]);
    }

  for (cnt = 0; cnt < nr_t; cnt++)
    pthread_join (pts[cnt], NULL);

  for (cnt = 0; cnt < nr_t; cnt++)
    spe_context_destroy (ctx[cnt]);

  return 0;
}

