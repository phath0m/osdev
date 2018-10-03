/*
 * sys/i686/kern/sched.c
 * Scheduler implementation for i686 compatible devices
 */
#include <rtl/list.h>
#include <rtl/malloc.h>
#include <rtl/string.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/i686/portio.h>
#include <sys/i686/vm.h>

// remove me
#include <rtl/printf.h>

// list of processes to run
struct list run_queue;

// pointer to next process's address space
struct vm_space *   sched_curr_address_space;
struct proc *       sched_curr_proc;

uintptr_t           sched_curr_page_dir;

static void
init_pit(uint32_t frequency)
{
    uint32_t divisor = 1193180 / frequency;

    io_write_byte(0x43, 0x36);
    io_write_byte(0x40, divisor & 0xFF);
    io_write_byte(0x40, (divisor >> 8) & 0xFF);
}

uintptr_t
sched_get_next_proc(uintptr_t prev_esp)
{
    /* defined in sys/i686/kern/interrupt.c */
    extern void set_tss_esp0(uint32_t esp0);

    struct proc *next_proc = NULL;

    if (sched_curr_proc) {
        sched_curr_proc->stack = prev_esp;

        if (sched_curr_proc->state == SRUN) {
            list_append(&run_queue, sched_curr_proc);
        }
    }

    if (list_remove_front(&run_queue, (void**)&next_proc) && next_proc != sched_curr_proc) {
        sched_curr_proc = next_proc;
        sched_curr_page_dir = (uintptr_t)next_proc->address_space->state_physical;
        //printf("at %p\n", next_proc);        
        //set_tss_esp0(next_proc->stack);
        //
        return next_proc->stack;
    }

    return prev_esp;
}

uintptr_t
sched_init_thread(struct vm_space *space, uintptr_t stack_start, kthread_entry_t entry, void *arg)
{
    uintptr_t stack_top = (uintptr_t)vm_share(sched_curr_address_space, space, NULL, (void*)stack_start, 0x1000, PROT_READ | PROT_WRITE | PROT_KERN);

    printf("stack top is at %p in our address space\n", stack_top);
    uint32_t *stack = (uint32_t*)stack_top;

    *--stack = (uint32_t)arg;
    *--stack = 0;
    *--stack = 0x202;
    *--stack = 0x08;
    *--stack = (uint32_t)entry;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0x10;
    *--stack = 0x10;
    *--stack = 0x10;
    *--stack = 0x10;

    uint32_t delta = (uint32_t)stack_top - (uint32_t)stack;

    vm_unmap(sched_curr_address_space, (void*)(stack_top & 0xFFFFF000), 0x1000);

    return (uintptr_t)(stack_start - delta);
}

void
sched_run_kthread(kthread_entry_t entrypoint)
{
    struct proc *proc = (struct proc*)calloc(0, sizeof(struct proc));

    proc->address_space = vm_space_new();
    proc->state = SRUN;

    vm_map(proc->address_space, (void*)0xFFFFF000, 1, PROT_WRITE | PROT_READ | PROT_KERN);
    
    proc->stack = sched_init_thread(proc->address_space, 0xFFFFFA00, entrypoint, NULL);
    printf("proc was allocated at %x\n", proc);   
    list_append(&run_queue, proc);

}

__attribute__((constructor)) static void
_init_pit_timer()
{
    init_pit(1000);

    sched_curr_address_space = vm_space_new();

    memset(&run_queue, 0, sizeof(struct list));

    asm volatile("mov %0, %%cr3" :: "r"((uint32_t)sched_curr_address_space->state_physical));
    asm volatile("mov %cr3, %eax; mov %eax, %cr3;");
}
