/*
 * world.c
 *
 * Allows the control of "worlds" (Basically jails/containers)
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
#include <ds/list.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/world.h>


int
kern_worldctl(struct proc *proc, int opcode, int arg)
{
    switch (opcode) {
        case WORLD_CREATE:
            proc_leave_world(proc, proc->world);
            proc->world = world_new(proc, arg);
            return 0;
        default:
            return -(EINVAL);
    }    
}

struct world *
world_new(struct proc *leader, int flags)
{
    struct world *world;

    world = calloc(1, sizeof(struct world));

    world->leader = leader;
    world->world_id = leader->pid;
    world->flags = flags;

    list_append(&world->members, leader);

    return world;
}