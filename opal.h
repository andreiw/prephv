/*
 * OPAL stuff.
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

#ifndef OPAL_H
#define OPAL_H

#define OPAL_SUCCESS            0
#define OPAL_PARAMETER          -1
#define OPAL_BUSY               -2
#define OPAL_PARTIAL            -3
#define OPAL_CONSTRAINED        -4
#define OPAL_CLOSED             -5
#define OPAL_HARDWARE           -6
#define OPAL_UNSUPPORTED        -7
#define OPAL_PERMISSION         -8
#define OPAL_NO_MEM             -9
#define OPAL_RESOURCE           -10
#define OPAL_INTERNAL_ERROR     -11
#define OPAL_BUSY_EVENT         -12
#define OPAL_HARDWARE_FROZEN    -13

#define OPAL_CONSOLE_WRITE       1
#define OPAL_CONSOLE_READ        2
#define OPAL_TERMINAL_0          0

#define OPAL_POLL_EVENTS         10

#define OPAL_REINIT_CPUS         70
#define OPAL_REINIT_CPUS_HILE_LE (1 << 1)

#ifndef __ASSEMBLY__
#include <mmu.h>

typedef long opal_return_t;

/*
 * I'm not obvious enough below - since OPAL runs with MMU off,
 * all pointer parameters must be real addresses or be HV real aliases.
 */
opal_return_t opal_write(int terminal, ra_t len, ra_t buf);
opal_return_t opal_read(int terminal, ra_t len, ra_t buf);
opal_return_t opal_reinit_cpus(uint64_t flags);
opal_return_t opal_poll_events(ra_t outstanding_event_mask);

#endif /* __ASSEMBLY__ */
#endif /* OPAL_H */
