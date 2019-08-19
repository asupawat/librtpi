/* SPDX-License-Identifier: LGPL-2.1-only */
/* Copyright © 2018 VMware, Inc. All Rights Reserved. */

#ifndef RPTI_H_INTERNAL_H
#define RPTI_H_INTERNAL_H

#include <linux/futex.h>

/*
 * PI Mutex
 */
union pi_mutex {
	struct {
		__u32	futex;
		__u32	flags;
	};
	__u8 pad[64];
} __attribute__ ((aligned(64)));

#define PI_MUTEX_INIT(f) { .futex = 0, .flags = f }

#define pi_mutex_lock_save(mutex, state)		\
({							\
	int ret = pi_mutex_trylock(mutex);		\
	state = false;					\
	if (ret) {					\
		if (ret == EDEADLOCK) {			\
			state = true;			\
			ret = 0;			\
		} else	{				\
			ret = futex_lock_pi(mutex);	\
			ret = (ret) ? errno : 0;	\
		}					\
	}						\
	ret;						\
})

#define pi_mutex_unlock_restore(mutex, state)		\
({							\
	int ret = 0;					\
	if (!state)					\
		ret = pi_mutex_unlock(mutex);		\
	ret;						\
})

/*
 * PI Cond
 */
union pi_cond {
	struct {
		union pi_mutex	*mutex;
		__u32		cond;
		__u32		flags;
		__u32		wake_id;
	};
	__u8 pad[128];
} __attribute__ ((aligned(64)));

#define PI_COND_INIT(m, f) \
	{ .mutex = m \
	, .cond = 0 \
	, .flags = f \
	, .wake_id = 0 }

#endif // RPTI_H_INTERNAL_H
