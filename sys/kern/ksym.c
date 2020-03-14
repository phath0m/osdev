/*
 * ksym.c
 *
 * Handles resolution of kernel symbols. Kernel symbols must be declared by the
 * machine dependent portion of the kernel by calling ksym_declare()
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
#include <ds/dict.h>
#include <machine/elf32.h>
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/malloc.h>

struct ksym {
    char        name[255];
    uintptr_t   value;
};

struct dict ksym_all;

void
ksym_declare(const char *name, uintptr_t val)
{
    struct ksym *sym = calloc(1, sizeof(struct ksym));
    sym->value = val;
    strncpy(sym->name, name, sizeof(sym->name));
    dict_set(&ksym_all, name, sym);
}

int
ksym_resolve(const char *name, uintptr_t *val)
{
    struct ksym *res;

    if (dict_get(&ksym_all, name, (void**)&res)) {
        *val = res->value;
        return 0;
    }

    return -1;
}


int
ksym_find_nearest(uintptr_t needle, uintptr_t *offset, char *buf, size_t bufsize)
{
    list_iter_t iter;

    dict_get_keys(&ksym_all, &iter);

    struct ksym *closest = NULL;
    char *name = NULL;

    while (iter_move_next(&iter, (void**)&name)) {
        struct ksym *ksym;

        if (!dict_get(&ksym_all, name, (void**)&ksym)) {
            continue;
        }

        if (ksym->value > needle) {
            continue;
        }

        if (!closest) {
            closest = ksym;
            continue;
        }

        uintptr_t delta = needle - ksym->value;

        if (delta < needle - closest->value) {
            closest = ksym;
        }
    }

    iter_close(&iter);

    if (!closest) {
        return -1;
    }

    strncpy(buf, closest->name, bufsize);

    if (offset) {
        *offset = needle - closest->value;
    }

    return 0;
}
