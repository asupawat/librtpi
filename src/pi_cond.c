// SPDX-License-Identifier: LGPL-2.1-only
// Copyright © 2018 VMware, Inc. All Rights Reserved.

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "rtpi.h"
#include "pi_futex.h"

/*
 * This wrapper for early library validation only.
 * TODO: Replace with pthread_cond_t wrapper with a new cond implementation.
 *       Base this on the older version of the condvar, with the patch from
 *       Dinakar and Darren to enable priority fifo wakeup order.
 */

pi_cond_t *pi_cond_alloc(void)
{
	return malloc(sizeof(pi_cond_t));
}

void pi_cond_free(pi_cond_t *cond)
{
	free(cond);
}

int pi_cond_init(pi_cond_t *cond, pi_mutex_t *mutex, uint32_t flags)
{
	struct timespec ts = { 0, 0 };
	int ret;

	if (flags & ~(RTPI_COND_PSHARED)) {
		ret = EINVAL;
		goto out;
	}
	memset(cond, 0, sizeof(*cond));
	if (flags & RTPI_COND_PSHARED) {
		cond->flags = RTPI_COND_PSHARED;
	}

	/* PSHARED has to match on both. */
	if ((cond->flags & RTPI_COND_PSHARED) ^ (mutex->flags % RTPI_MUTEX_PSHARED)) {
		ret = EINVAL;
		goto out;
	}
	cond->mutex = mutex;

	ret = 0;
out:
	return ret;
}

int pi_cond_timedwait(pi_cond_t *cond, const struct timespec *restrict abstime)
{
	int ret;
	int err;
	__u32 wait_id;
	__u32 futex_id;

	cond->cond++;
	wait_id = cond->cond;
  again:
	futex_id = cond->cond;
	ret = pi_mutex_unlock(cond->mutex);
	if (ret)
		return ret;

	ret = futex_wait_requeue_pi(cond, futex_id, abstime, cond->mutex);
	err = errno;

	/* All good. Proper wakeup + we own the lock */
	if (!ret)
		return 0;

	/* For error cases we need to re-acquire the mutex. */
	ret = pi_mutex_lock(cond->mutex);
	if (ret)
		return ret;

	/* If futex VAL changed between unlock & wait. */
	if (err == EAGAIN) {
		/* Check if we raced with a waker. */
		if (cond->wake_id >= wait_id)
			return 0;

		/* Reload VAL and try again */
		cond->cond++;
		goto again;
	}

	return err;
}

int pi_cond_wait(pi_cond_t *cond)
{
	return pi_cond_timedwait(cond, NULL);
}

static int pi_cond_signal_common(pi_cond_t *cond, bool broadcast)
{
	int ret;
	__u32 id;

again:
	cond->cond++;
	id = cond->cond;
	cond->wake_id = id;

	ret = futex_cmp_requeue_pi(cond, id,
				   (broadcast) ? INT_MAX : 0,
				   cond->mutex);
	if (ret >= 0)
		return ret;

	if (errno == EAGAIN)
		goto again;

	return -errno;
}

int pi_cond_signal(pi_cond_t *cond)
{
	int ret = pi_cond_signal_common(cond, false);

	return (ret >= 0) ? 0 : -ret;
}

int pi_cond_broadcast(pi_cond_t *cond)
{
	int ret = pi_cond_signal_common(cond, true);

	return (ret >= 0) ? 0 : -ret;
}

int pi_cond_destroy(pi_cond_t *cond)
{
	int ret;
	bool state;

	ret = pi_mutex_lock_save(cond->mutex, state);
	if (ret)
		return ret;

	ret = pi_cond_signal_common(cond, true);

	pi_mutex_unlock_restore(cond->mutex, state);

	if (ret == 0) {
		memset(cond, 0, sizeof(*cond));
		return 0;
	}

	return (ret > 0) ? EBUSY : -ret;
}
