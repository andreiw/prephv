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

#include <log.h>
#include <console.h>
#include <vsprintf.h>

#define ANSI_CSI              "\33["
#define ANSI_BLACK_FG         30
#define ANSI_RED_FG           31
#define ANSI_GREEN_FG         32
#define ANSI_YELLOW_FG        33
#define ANSI_BLUE_FG          34
#define ANSI_MGENTA_FG        35
#define ANSI_CYAN_FG          36
#define ANSI_WHITE_FG         37
#define ANSI_BLACK_BRIGHT_FG  90
#define ANSI_RED_BRIGHT_FG    91
#define ANSI_GREEN_BRIGHT_FG  92
#define ANSI_YELLOW_BRIGHT_FG 93
#define ANSI_BLUE_BRIGHT_FG   94
#define ANSI_MGENTA_BRIGHT_FG 95
#define ANSI_CYAN_BRIGHT_FG   96
#define ANSI_WHITE_BRIGHT_FG  97
#define ANSI_BLACK_BG         40
#define ANSI_RED_BG           41
#define ANSI_GREEN_BG         42
#define ANSI_YELLOW_BG        43
#define ANSI_BLUE_BG          44
#define ANSI_MGENTA_BG        45
#define ANSI_CYAN_BG          46
#define ANSI_WHITE_BG         47

#define ANSI_FG(color)        color##_FG
#define ANSI_BRIGHT_FG(color) color##_BRIGHT_FG
#define ANSI_BG(color)        color##_BG
#define ANSI_NORMAL           0
#define DEREF(x) x
#define ANSI_SET(color)       ANSI_CSI SIFY(color) "m"
#define ANSI_RESET            ANSI_SET(ANSI_NORMAL)

static char *colors[LOG_MAX] = {
	ANSI_SET(ANSI_BRIGHT_FG(ANSI_RED)),
	ANSI_SET(ANSI_BRIGHT_FG(ANSI_RED)),
	ANSI_SET(ANSI_BRIGHT_FG(ANSI_YELLOW)),
	ANSI_SET(ANSI_BRIGHT_FG(ANSI_WHITE)),
	ANSI_SET(ANSI_NORMAL),
	ANSI_SET(ANSI_BRIGHT_FG(ANSI_BLACK)),
};

/*
 * Print a log message.
 */
void
_log(unsigned level, unsigned log_lvl, char *pfx, char *fmt, ...)
{
	unsigned i;
	va_list ap;
	char buf[512];

	if (level > log_lvl &&
	    level != LOG_ERROR &&
	    level != LOG_FATAL) {
		return;
	} else if (level > LOG_MAX) {
		level = LOG_MAX - 1;
	}

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	con_puts(colors[level]);

	for (i = 0; i < ARRAY_LEN(buf) && buf[i] != '\0'; i++) {
		if (i == 0 || buf[i] == '\n') {
			if (buf[i] == '\n') {
				con_putchar(buf[i]);
			}

			con_puts(ANSI_SET(ANSI_BRIGHT_FG(ANSI_BLACK)) "[");
			con_puts(pfx);
			con_putchar(']');
			con_puts(colors[level]);

			if (buf[i] == '\n') {
				continue;
			}
		}

		con_putchar(buf[i]);
	}

	con_puts(ANSI_RESET "\n");
	if (level == LOG_FATAL) {
		con_puts(ANSI_SET(ANSI_BG(ANSI_RED)));
		con_puts(ANSI_SET(ANSI_FG(ANSI_BLACK)));
		con_puts("Fatal prephv error detected.");
		con_puts(ANSI_RESET "\n");

		// XXX
		while(1);
	}
}
