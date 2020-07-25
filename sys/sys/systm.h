/*
 * systm.h
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
#ifndef _SYS_SYSTM_H
#define _SYS_SYSTM_H
#ifdef __cplusplus
extern "C" {
#endif
#ifdef __KERNEL__
#include <stdarg.h>
#include <sys/cdev.h>
#include <sys/vnode.h>

#define ELYSIUM_SYSNAME     "Elysium"
#define ELYSIUM_RELEASE     "v0.0.1"
#define ELYSIUM_VERSION     __DATE__ " "__TIME__ 

#define KASSERT(expr, msg) \
    if (!(expr)) { \
        panic("kassert: %s", msg); \
    }

void    critical_enter();
void    critical_exit();

int     kmain(const char *);
void    puts(const char *);
void    set_kernel_output(struct cdev *);
void    panic(const char *, ...);
void    shutdown();
void    stacktrace(int);
void    printf(const char *, ...);
void    vprintf(const char *, va_list);

void    ksym_declare(const char *, uintptr_t);
int     ksym_find_nearest(uintptr_t, uintptr_t *, char *, size_t);
int     ksym_resolve(const char *, uintptr_t *);

#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif
#endif /* _SYS_SYSTM_H */
