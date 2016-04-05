/*
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

#ifndef STRING_H
#define STRING_H

#include <types.h>

int strnicmp(const char *s1, const char *s2, length_t len);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, length_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, length_t count);
length_t strlcpy(char *dest, const char *src, length_t size);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, length_t count);
length_t strlcat(char *dest, const char *src, length_t count);
int strcmp(const char *cs, const char *ct);
int strncmp(const char *cs, const char *ct, length_t count);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strnchr(const char *s, length_t count, int c);
char *skip_spaces(const char *str);
char *strim(char *s);
length_t strlen(const char *s);
length_t strnlen(const char *s, length_t count);
length_t strspn(const char *s, const char *accept);
length_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *cs, const char *ct);
char *strsep(char **s, const char *ct);
void *memset(void *s, int c, length_t count);
void *memcpy(void *dest, const void *src, length_t count);
void *memmove(void *dest, const void *src, length_t count);
int memcmp(const void *cs, const void *ct, length_t count);
void *memscan(void *addr, int c, length_t size);
char *strstr(const char *s1, const char *s2);
char *strnstr(const char *s1, const char *s2, length_t len);
void *memchr(const void *s, int c, length_t n);

extern const char hex_asc[];
#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]

static inline char *pack_hex_byte(char *buf, uint8_t byte)
{
        *buf++ = hex_asc_hi(byte);
        *buf++ = hex_asc_lo(byte);
        return buf;
}

#endif
