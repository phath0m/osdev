/*
 * signal.h
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
#ifndef _ELYSIUM_SYS_SIGNAL_H
#define _ELYSIUM_SYS_SIGNAL_H
#ifdef __cplusplus
extern "C" {    
#endif

#include <sys/types.h>

typedef unsigned int sigset_t;

struct sigaction {
    uintptr_t   sa_handler;
    uintptr_t   sa_sigaction;
    sigset_t    sa_mask;
    int         sa_flags;
    uintptr_t   sa_restorer;
};

struct signal_args {
    uintptr_t       handler;
    uintptr_t       arg;
};

struct sighandler {
    uintptr_t   handler;
    uintptr_t   arg;
    int         mask;
    int         flags;
};

struct regs;

struct sigcontext {
    bool            invoked;
    uintptr_t       handler_func;
    uintptr_t       restorer_func;
    uintptr_t       arg;
    int             signo;
    struct regs *   regs;
};

#ifdef __cplusplus
}
#endif
#endif /* _ELYSIUM_SYS_SIGNAL_H */
