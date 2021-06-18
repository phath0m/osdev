/*
 * types.h
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#define NULL    0
#define true    1
#define false   0

typedef unsigned char bool;

typedef unsigned int size_t;
typedef int ssize_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

typedef short dev_t;
typedef int mode_t;
typedef unsigned short nlink_t;
typedef unsigned short uid_t;
typedef unsigned short gid_t;
typedef int pid_t;

typedef unsigned int ino_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;
typedef long long int int64_t;
typedef unsigned long int time_t;

typedef int intptr_t;
typedef unsigned int uintptr_t;

typedef signed long long int off_t;

#endif
