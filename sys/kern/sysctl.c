/*
 * sysctl.c
 *
 * Dispatches sysctl requests to their appropriate subsystem based on the
 * root OID supplied in the name argument.
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
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/types.h>

int
kern_sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case KERN_PROC:
            return proc_sysctl(&name[1], namelen - 1, oldp, oldlenp, newp, newlen);
    }

    return -1;
}

int
sysctl(int *name, int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
    switch (name[0]) {
        case CTL_KERN:
            return kern_sysctl(&name[1], namelen - 1, oldp, oldlenp, newp, newlen);    
    }
    
    return -1;
}

