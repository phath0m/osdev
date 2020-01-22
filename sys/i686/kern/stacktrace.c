#include <stdio.h>
#include <sys/types.h>

struct stackframe {
    struct stackframe * prev;
    uintptr_t           eip;
};

void
stacktrace(int max_frames)
{
    struct stackframe *frame;

    asm volatile("movl %%ebp, %%edx": "=d"(frame));
    printf("Trace:\n");

    for (int i = 0; i < max_frames && frame; i++) {
        printf("    [0x%p]\n", frame->eip);
        frame = frame->prev;
    }
}
