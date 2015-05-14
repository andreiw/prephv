/*
 * Dumb printing routines
 *
 * Copyright (C) Andrey Warkentin <andrey.warkentin@gmail.com>
 * Copyright (C) 1996 Pete A. Zaitcev
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
#include <stdarg.h>
#include <stddef.h>
#include <endian.h>
#include <console.h>


void
putchar(char c)
{
	u64 len = cpu_to_be64(1);
	opal_write(OPAL_TERMINAL_0, &len, &c);
}


/*
 * Print a string.
 */
static void
printks(char *s)
{
   char c;
   char *null_string = "<NULL>";

   if (s == NULL) {
      s = null_string;
   }

   while ((c = *s++) != '\0') {
      putchar(c);
   }
}


/*
 * Print an unsigned integer in base b, avoiding recursion.
 */
static void
printknu(u64 n, u64 b)
{
   char prbuf[24];
   register char *cp;

   cp = prbuf;
   do {
      *cp++ = "0123456789ABCDEF"[(int) (n % b)];
   } while ((n = n / b & 0x0FFFFFFFFFFFFFFF) != 0);

   do {
      putchar (*--cp);
   } while (cp > prbuf);
}


/*
 * Print an signed integer in base b, avoiding recursion.
 */
static void
printkns(u64 n, u64 b)
{
   char prbuf[24];
   register char *cp;

   if (n < 0) {
      putchar ('-');
      n = -n;
   }

   cp = prbuf;
   do {
      *cp++ = "0123456789ABCDEF"[(int) (n % b)];
   } while ((n = n / b & 0x0FFFFFFFFFFFFFFF) != 0);

   do {
      putchar (*--cp);
   } while (cp > prbuf);
}


void
vprintk(char *fmt, va_list adx)
{
   char *s;
   char c;

   for (;;) {
      while ((c = *fmt++) != '%') {
         if (c == '\0') {
            putchar(0);
            return;
         }

         putchar(c);
      }

      c = *fmt++;
      if (c == 'u' || c == 'o' ||
          c == 'x' || c == 'X') {
         printknu((u64) va_arg(adx, u64),
                  c == 'o' ? 8 : (c == 'u' ? 10 : 16));
      } else if (c == 'i' || c == 'd')  {
         printkns((u64) va_arg(adx, u64), 10);
      } else if (c == 'c') {
         putchar(va_arg(adx, int));
      } else if (c == 's') {
         s = va_arg(adx, char *);
         printks(s);
      } else if (c == 'p') {
         s = va_arg(adx, void *);
         if (s) {
            putchar('0');
            putchar('x');
            printknu((u64) s, 16);
         } else {
            printks(NULL);
         }
      }
   }
}


/*
 * Scaled down version of C Library printf.
 * Only %c %s %u %d %i %o %x %p are recognized,
 * and %u %d %i %o and %x operate on 64-bit values.
 */
void
printk(char *fmt,...)
{
   va_list x1;

   va_start(x1, fmt);
   vprintk(fmt, x1);
   va_end(x1);
}
