#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "thread.h"

static int
new_thread_wrap_func(void *arg)
{
    thread_t *thread = (thread_t*)arg;

    thread->start_routine(thread->arg);

    exit(0);
    
    return 0;
}

extern int clone(int (*func)(), void *stack, int flags, void *arg);

void
thread_create(thread_t *thread, void *(*start_routine) (void *), void *arg)
{
    uintptr_t stack = (uintptr_t)calloc(1, 65535);
    
    void *stack_top = (void*)((uintptr_t)stack + 65530);

    static thread_t state;

    state.start_routine = start_routine;
    state.arg = arg;

    clone(new_thread_wrap_func, stack_top, 3, &state); 
    
}
