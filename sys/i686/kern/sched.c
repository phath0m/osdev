/*
 * sys/i686/kern/sched.c
 * Scheduler implementation for i686 compatible devices
 */
#include <stdlib.h>
#include <string.h>
#include <ds/list.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/i686/interrupt.h>
#include <sys/i686/portio.h>
#include <sys/i686/vm.h>
// remove
#include <stdio.h>

// list of processes to run
struct list         run_queue;
struct list         dead_threads;

struct proc *       current_proc;
struct vm_space *   sched_curr_address_space;
struct thread *     sched_curr_thread;

uintptr_t           sched_curr_page_dir;

static void
init_pit(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;

    io_write_byte(0x43, 0x36);
    io_write_byte(0x40, divisor & 0xFF);
    io_write_byte(0x40, (divisor >> 8) & 0xFF);
}

static void
sched_reap_threads()
{
    list_iter_t iter;

    list_get_iter(&dead_threads, &iter);

    struct thread *thread;

    while (iter_move_next(&iter, (void**)&thread)) {
        proc_destroy(thread->proc);
    }

    iter_close(&iter);
    
    list_destroy(&dead_threads, true);
}


int
sched_get_next_proc(uintptr_t prev_esp)
{
    /* defined in sys/i686/kern/interrupt.c */
    extern void set_tss_esp0(uint32_t esp0);

    struct thread *next_thread = NULL;

    if (list_remove_front(&run_queue, (void**)&next_thread) && next_thread != sched_curr_thread) {
        
        if (sched_curr_thread) {
            sched_curr_thread->stack = prev_esp;

            if (sched_curr_thread->state == SRUN) {
                
                if (LIST_SIZE(&dead_threads) > 0) {
                    sched_reap_threads();
                }

                list_append(&run_queue, sched_curr_thread);
            }
        }

        sched_curr_address_space = next_thread->address_space;
        sched_curr_thread = next_thread;
        sched_curr_page_dir = (uintptr_t)next_thread->address_space->state_physical;

        current_proc = next_thread->proc;

        return next_thread->stack;
    }

    if (sched_curr_thread) {
        current_proc = sched_curr_thread->proc;
    }

    return prev_esp;
}


uintptr_t
sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg)
{
    uintptr_t stack_top = (uintptr_t)vm_share(sched_curr_address_space, space, NULL, (void*)stack_start, 0x1000, PROT_READ | PROT_WRITE | PROT_KERN);

    uint32_t *stack = (uint32_t*)stack_top;

    *--stack = (uint32_t)arg;
    *--stack = stack_top; /* idk (I think ESP?)) */
    *--stack = 0x202; /* EFLAGS???? */
    *--stack = 0x08; /* code segment */
    *--stack = (uint32_t)entry;
    *--stack = 0;       /* EAX */
    *--stack = 0;       /* ECX */
    *--stack = 0;       /* EDX */
    *--stack = 0;       /* EBX */
    *--stack = 0;       /* reserved */
    *--stack = 0;       /* EBP */
    *--stack = 0;       /* ESI */
    *--stack = 0;       /* EDI */
    *--stack = 0x10;    /* GS */
    *--stack = 0x10;    /* ES */
    *--stack = 0x10;    /* FS */
    *--stack = 0x10;    /* GS  */

    uint32_t delta = (uint32_t)stack_top - (uint32_t)stack;

    vm_unmap(sched_curr_address_space, (void*)(stack_top & 0xFFFFF000), 0x1000);

    return (uintptr_t)(stack_start - delta);
}

void
sched_run_kthread(kthread_entry_t entrypoint, struct vm_space *space, void *arg)
{
    struct thread *thread = (struct thread*)calloc(0, sizeof(struct thread));

    if (space) {
        thread->address_space = space;
    } else {
        thread->address_space = vm_space_new();
    }

    thread->state = SRUN;

    vm_map(thread->address_space, (void*)0xFFFFF000, 1, PROT_WRITE | PROT_READ | PROT_KERN);
    
    thread->stack = sched_init_thread(thread->address_space, 0xFFFFFA00, entrypoint, arg);

    list_append(&run_queue, thread);
}

void
sched_yield()
{
    asm volatile("sti");
    asm volatile("hlt");
}

void
schedule_thread(int state, struct thread *thread)
{
    thread->state = state;

    if (state == SRUN) {
        list_append(&run_queue, thread);
    } else if (state == SDEAD) {
        list_append(&dead_threads, thread);
    }
}

__attribute__((constructor))
void
_init_pit_timer()
{
    init_pit(1000);

    sched_curr_address_space = vm_space_new();

    memset(&run_queue, 0, sizeof(struct list));

    printf("do it now! %p\n", sched_curr_address_space);
    printf("physical   %p\n", sched_curr_address_space->state_physical);

    asm volatile("mov %0, %%cr3" :: "r"((uint32_t)sched_curr_address_space->state_physical));
    asm volatile("mov %cr3, %eax; mov %eax, %cr3;");
}
