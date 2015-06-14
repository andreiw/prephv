/*
 * Timebase and decrementer.
 *
 * Copyright (C) 2015 Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef TIME_H
#define TIME_H

#include <kpcr.h>
#include <list.h>

struct time_req_s;
typedef bool_t (*time_cb)(struct time_req_s *t);

typedef struct time_req_s
{
	struct list_head link;
	char *name;
	void *ctx;
	uint64_t when;
	time_cb cb;
} time_req_t;

static inline uint64_t
secs_to_tb(uint64_t secs)
{
	return secs * kpcr_get()->tb_freq;
}

static inline uint64_t
ms_to_tb(uint64_t ms)
{
	return ms * (kpcr_get()->tb_freq / 1000);
}

static inline uint64_t
tb_to_secs(uint64_t tb)
{
	return tb / kpcr_get()->tb_freq;
}

static inline uint64_t
tb_to_ms(uint64_t tb)
{
	return tb / (kpcr_get()->tb_freq / 1000);
}

void time_init(void);
void time_handle(void);
void time_delay(uint64_t delay);
void time_prep(uint64_t when, time_cb cb, char *n, void *ctx, time_req_t *t);
void time_enqueue(time_req_t *t);
void time_dequeue(time_req_t *t);

static inline void
time_prep_s(uint64_t secs, time_cb cb, char *n, void *ctx, time_req_t *t)
{
	return time_prep(secs_to_tb(secs) + mftb(), cb, n, ctx, t);
}

static inline void
time_prep_ms(uint64_t ms, time_cb cb, char *n, void *ctx, time_req_t *t)
{
	return time_prep(ms_to_tb(ms) + mftb(), cb, n, ctx, t);
}

#endif /* TIME_H */
