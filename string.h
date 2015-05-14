/*
 * String ops.
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

#ifndef STRING_H
#define STRING_H

#include <types.h>

char *strstr(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, length_t n);
char *strcat(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, length_t n);
length_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
void *memchr(const void *s, int c, length_t n);
void *memset(void *s, int c, length_t n);
void bcopy(const void *src, void *dest, length_t n);
void *memcpy(void *dest, const void *src, length_t n);
void *memmove(void *dest, const void *src, length_t n);
int memcmp(const void *s1, const void *s2, length_t n);

#endif /* STRING_H */
