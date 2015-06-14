/*
 * Timebase and decrementer support.
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

#include <ppc.h>
#include <time.h>
#include <opal.h>
#include <list.h>
#include <console.h>
#include <exc.h>

/*
 * This is a really hopeless timer implementation. A crappy toy :(.
 */
struct list_head reqs;


static
void time_enqueue_int(time_req_t *t)
{
	time_req_t *pos;

	list_for_each_entry(pos, &reqs, link) {
		if (pos == t) {
			/*
			 * Already added.
			 */
			return;
		}

		if (pos->when > t->when) {
			/*
			 * Added t before pos.
			 */
			list_add_tail(&t->link, &pos->link);
			return;
		}
	}

	/*
	 * Add to end of list.
	 */
	list_add_tail(&t->link, &reqs);
}


void
time_handle(void)
{
	time_req_t *t;
	time_req_t *n;
	uint64_t dec;
	uint64_t now = mftb();
	unsigned handled = 0;

	list_for_each_entry_safe(t, n, &reqs, link) {
		now = mftb();

		if (t->when <= now) {
			list_del(&t->link);
			handled++;

			if (t->cb(t)) {
				/*
				 * Enqueue back.
				 */
				BUG_ON(t->when <= now,
				       "enqueuing stale timer %p", t);
				time_enqueue_int(t);
				if (&n->link != &reqs) {
					if (t->when < n->when) {
						n = t;
					}
				} else {
					/*
					 * The re-enqueued timer is next.
					 */
					break;
				}
			}
		} else {
			break;
		}
	}

	dec = DEC_DISABLE;
	if (&t->link != &reqs &&
	    (t->when - now) < DEC_DISABLE) {
		dec = t->when - now;
	}

	/*
	 * printk("%u timers fired, next wakeup 0x%x\n", handled, dec);
	 */
	set_DEC(dec);
}


void
time_prep(uint64_t when,
          time_cb cb,
          char *n,
          void *ctx,
          time_req_t *t)
{
	INIT_LIST_HEAD(&t->link);

	t->when = when;
	t->cb = cb;
	t->ctx = ctx;
	t->name = n;
}


void
time_enqueue(time_req_t *t)
{
	exc_flags_t f;

	/*
	 * UP only...
	 */
	f = exc_disable_ee();
	time_enqueue_int(t);
	exc_restore_ee(f);
	time_handle();
}


void
time_dequeue(time_req_t *t)
{
	exc_flags_t f;
	time_req_t *pos;
	time_req_t *n;

	/*
	 * UP only...
	 */
	f = exc_disable_ee();
	list_for_each_entry_safe(pos, n, &reqs, link) {
		if (pos == t) {
			list_del(&t->link);
			break;
		}
	}
	exc_restore_ee(f);
}


void
time_delay(uint64_t delay)
{
	uint64_t now = mftb();
	printk("Delaying for %u.%us\n",
	       delay / kpcr_get()->tb_freq,
	       delay % kpcr_get()->tb_freq);
	while ((mftb() - now) < delay) {
		cpu_relax();
		printk("Now = %u.%u s\n",
		       (mftb() - now) / kpcr_get()->tb_freq,
		       (mftb() - now) % kpcr_get()->tb_freq);
	}
	printk("Done!\n");
}


void
time_init(void)
{
	INIT_LIST_HEAD(&reqs);
}
