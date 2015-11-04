/*
 * prephv logging.
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

#ifndef LOG_H
#define LOG_H

#include <defs.h>

#ifndef LOG_PFX
#define LOG_PFX prephv
#endif /* LOG_PFX */

#define LOG_FATAL   0
#define LOG_ERROR   1
#define LOG_WARN    2
#define LOG_NORMAL  3
#define LOG_VERBOSE 4
#define LOG_DEBUG   5
#define LOG_MAX     6

#ifndef LOG_LVL
#define LOG_LVL LOG_NORMAL
#endif /* LOG_LVL */

void _log(unsigned level, unsigned log_lvl, char *pfx, char *fmt, ...);
#define _LOG(level, fmt, ...) _log(level, LOG_LVL, SIFY(LOG_PFX), fmt, ## __VA_ARGS__)

#define FATAL(fmt, ...)   _LOG(LOG_FATAL, fmt, ## __VA_ARGS__)
#define WARN(fmt, ...)     _LOG(LOG_WARN, fmt, ## __VA_ARGS__)
#define ERROR(fmt, ...)   _LOG(LOG_ERROR, fmt, ## __VA_ARGS__)
#define LOG(fmt, ...)     _LOG(LOG_NORMAL, fmt, ## __VA_ARGS__)

#ifdef OPTIMIZED
#define VERBOSE(fmt, ...) _LOG(LOG_VERBOSE, fmt, ## __VA_ARGS__)
#define DEBUG(fmt, ...)
#else /*  OPTIMIZED */
#define VERBOSE(fmt, ...) _LOG(LOG_VERBOSE, fmt, ## __VA_ARGS__)
#define DEBUG(fmt, ...)   _LOG(LOG_DEBUG, fmt, ## __VA_ARGS__)
#endif /* Not OPTIMIZED */

#endif /* LOG_H */
