/*
 * procdesc.h
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
#ifndef _ELYSIUM_SYS_PROCDEC_H
#define _ELYSIUM_SYS_PROCDEC_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__

#include <sys/file.h>

struct file *   procdesc_getfile(int);
int             procdesc_getfd();
int             procdesc_newfd(struct file *);
int             procdesc_dup(int); 
int             procdesc_dup2(int, int);
int             procdesc_fcntl(int, int, void *);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_PROCDEC_H */
