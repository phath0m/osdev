/*
 * fork.c - fork() implementation
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
#include <machine/reg.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/world.h>

struct fork_state {
    struct proc *   proc;
    uintptr_t       u_stack_top;
    uintptr_t       u_stack_bottom;
    bool            initialized;
    struct regs     regs;
};

static void
copy_image(struct proc *proc, struct vm_space *new_space)
{
    ssize_t image_size;
    void *image;

    image_size = ((proc->brk - proc->base));

    KASSERT("proc image size should not be zero or negative", image_size > 0);

    vm_map(new_space, (void*)proc->base, image_size, VM_READ | VM_WRITE);

    image = vm_share(proc->thread->address_space, new_space, NULL, (void*)proc->base,
                        image_size, VM_READ | VM_WRITE | VM_KERN);

    memcpy(image, (void*)proc->base, image_size);

    vm_unmap(proc->thread->address_space, image, image_size);
}

static void
copy_fildes(struct proc *proc, struct proc *new_proc)
{
    int i;

    for (i = 0; i < 4096; i++) {
        if (proc->files[i]) {
            new_proc->files[i] = file_duplicate(proc->files[i]);
        }
    }
}

static void
copy_stack(struct thread *thread, struct vm_space *new_space)
{
    size_t stack_size;
    void *stack;

    stack_size = thread->u_stack_top - thread->u_stack_bottom;

    vm_map(new_space, (void*)thread->u_stack_bottom, stack_size, VM_READ | VM_WRITE);

    stack = vm_share(thread->address_space, new_space, NULL, (void*)thread->u_stack_bottom,
                           stack_size, VM_READ | VM_WRITE | VM_KERN);
    memcpy(stack, (void*)thread->u_stack_bottom, stack_size);

    vm_unmap(thread->address_space, stack, stack_size);
}

static int
init_child_proc(void *statep)
{
    /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

    uintptr_t eip;
    uintptr_t esp;
    uintptr_t ebp;

    struct fork_state *state;
    struct proc *proc;

    bus_interrupts_off();

    state = statep;
    proc = state->proc;

    proc->thread = sched_curr_thread;
    
    sched_curr_thread->proc = state->proc;
    sched_curr_thread->u_stack_bottom = state->u_stack_bottom;
    sched_curr_thread->u_stack_top = state->u_stack_top;
    sched_curr_thread->tid = proc->pid;

    list_append(&proc->threads, sched_curr_thread);

    current_proc = proc;

    eip = state->regs.eip;
    esp = state->regs.uesp;
    ebp = state->regs.ebp;

    state->initialized = true;

    bus_interrupts_on();

    return_to_usermode(eip, esp, ebp, 0);
    
    return 0;
}

int
proc_fork(struct regs *regs)
{
    struct fork_state state;
    struct proc *proc;
    struct proc *new_proc;
    struct vm_space *new_space;

    bus_interrupts_off();

    proc = current_proc;
    new_proc = proc_new();
    new_space = vm_space_new();

    strncpy(new_proc->name, proc->name, sizeof(new_proc->name));
    memcpy(&new_proc->creds, &proc->creds, sizeof(struct cred));

    new_proc->base = proc->base;
    new_proc->brk = proc->brk;
    new_proc->cwd = proc->cwd;
    new_proc->parent = proc;
    new_proc->root = proc->root;
    new_proc->umask = proc->umask;
    new_proc->group = proc->group;
    new_proc->world = proc->world;

    VN_INC_REF(proc->root);
    VN_INC_REF(proc->cwd);

    list_append(&proc->children, new_proc);

    KASSERT(proc->group != NULL, "process cannot inherit a NULL group");

    list_append(&proc->group->members, new_proc);
    list_append(&proc->world->members, new_proc);

    copy_stack(proc->thread, new_space);
    copy_image(proc, new_space);
    copy_fildes(proc, new_proc);

    memset(&state, 0, sizeof(state));
    state.proc = new_proc;
    state.u_stack_top = proc->thread->u_stack_top;
    state.u_stack_bottom = proc->thread->u_stack_bottom;

    memcpy(&state.regs, regs, sizeof(struct regs));

    thread_run(init_child_proc, new_space, (void*)&state);

    bus_interrupts_on();

    while (!state.initialized) {
        thread_yield();
    }

    return new_proc->pid;
}
