/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef _RTPI_H
#define _RTPI_H

#include <inttypes.h>
#include <time.h>

/* TODO: Make these opaque types */
typedef struct pi_mutex {
	int nil;
} pi_mutex_t;

typedef struct pi_cond {
	int nil;
} pi_cond_t;

/*
 * PI Mutex Interface
 */
int pi_mutex_init(pi_mutex_t *mutex, uint32_t flags);

int pi_mutex_destroy(pi_mutex_t *mutex);

int pi_mutex_lock(pi_mutex_t *mutex);

int pi_mutex_trylock(pi_mutex_t *mutex);

int pi_mutex_unlock(pi_mutex_t *mutex);


/*
 * PI Cond Interface
 */
int pi_cond_init(pi_cond_t *cond, pi_mutex_t *mutex, uint32_t flags);

int pi_cond_destroy(pi_cond_t *cond);

int pi_cond_wait(pi_cond_t *cond);

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime);

int pi_cond_signal(pi_cond_t *cond);

int pi_cond_broadcast(pi_cond_t *cond);

#endif // _RTPI_H