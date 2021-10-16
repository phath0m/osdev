/*
 * exec.c - exec implementation
 *
 * Loads an ELF32 binary into the address space of the calling process
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
#include <machine/elf32.h>
#include <machine/vm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/interrupt.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vm.h>

static const char **
copy_strings(const char **strs, size_t *sizep, int *countp)
{
    int count;
    int i;
    size_t size;
    size_t ptrs_size;
    uintptr_t *str_ptrs;

    char *dest;
    void *buf;

    char const *src;

    for (count = 0, size = 0; strs[count]; count++) {
        size += strlen(strs[count]) + 1;
    }

    ptrs_size = (count + 1) * sizeof(char*);

    buf = calloc(1, size + ptrs_size);

    str_ptrs = (uint32_t*)buf;
    
    dest = (char*)((uintptr_t)buf + ptrs_size);
    
    for (i = 0; strs[i]; i++) {
        src = strs[i];

        strcpy(dest, src);

        str_ptrs[i] = (uintptr_t)dest;
        dest = &dest[strlen(src) + 1];
    }

    str_ptrs[count] = NULL;

    *sizep = size + ptrs_size;
    *countp = count;

    return (const char**)buf;
}

static void
realign_pointers(uintptr_t *ptr, size_t delta)
{
    while (*ptr) {
        *ptr -= delta;
        ptr++;
    }
}

static void
elf_get_dimensions(struct elf32_ehdr *hdr, uintptr_t *vlow, uintptr_t *vhigh)
{
    int i;
    uintptr_t high;
    uintptr_t low;

    struct elf32_phdr *phdr;
    struct elf32_phdr *phdrs;

    phdrs = elf_get_pheader(hdr);

    high = 0;
    low = 0xFFFFFFFF;

    for (i = 0; i < hdr->e_phnum; i++) {
        phdr = &phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            if (low > phdr->p_vaddr) {
                low = phdr->p_vaddr;
            }

            if (high < phdr->p_vaddr + phdr->p_memsz) {
                high = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

    *vlow = low;
    *vhigh = high;
}

static void
elf_load_image(struct vm_space *space, struct elf32_ehdr *hdr)
{
    int i;
    struct elf32_phdr *phdr;
    struct elf32_phdr *phdrs;
    
    phdrs = elf_get_pheader(hdr);

    for (i = 0; i < hdr->e_phnum; i++) {
        phdr = &phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            memcpy((void*)phdr->p_vaddr, ((void*)(uintptr_t)hdr + phdr->p_offset), phdr->p_filesz);
        }
    }
}

static void
elf_zero_sections(struct vm_space *space, struct elf32_ehdr *hdr)
{
    int i;

    struct elf32_shdr *shdr;
    struct elf32_shdr *shdrs;
    
    shdrs = elf_get_sheader(hdr);

    for (i = 0; i < hdr->e_shnum; i++) {
        shdr = &shdrs[i];

        if (shdr->sh_type == SHT_NOBITS) {

            if (!shdr->sh_size) {
                continue;
            }

            if (shdr->sh_flags & SHF_ALLOC) {
                memset((void*)shdr->sh_addr, 0, shdr->sh_size);
            }
        }
    }
}

void
unload_userspace(struct vm_space *space)
{
    list_iter_t iter;
    struct list to_remove;
    struct vm_block *block;

    memset(&to_remove, 0, sizeof(struct list));
    list_get_iter(&space->map, &iter);

    while (iter_move_next(&iter, (void**)&block)) {
        if (!(block->prot & VM_KERN)) {
            list_append(&to_remove, block);
        }
    }

    iter_close(&iter);
    list_get_iter(&to_remove, &iter);
    
    while (iter_move_next(&iter, (void**)&block)) {
        vm_unmap(space, (void*)block->start_virtual, 0x1000);
    }

    iter_close(&iter); 
    list_destroy(&to_remove, false);
}

int
proc_execve(const char *path, const char **argv, const char **envp)
{
    /* defined in sys/i686/kern/usermode.asm */
    extern void return_to_usermode(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

     /* defined in sys/i686/kern/sched.c */
    extern struct thread *sched_curr_thread;

    int argc;
    int envc;
    int i;

    size_t argv_size;
    size_t envp_size;
    size_t prog_size;
    size_t size;

    uintptr_t entrypoint;
    uintptr_t prog_low;
    uintptr_t prog_high;
    uintptr_t stack_bottom;
    uintptr_t stackp;
    uintptr_t u_argv;
    uintptr_t u_envp;
    
    const char **cp_argv;
    const char **cp_envp;

    struct elf32_ehdr *elf;
    struct file *exe;
    struct proc *proc;
    struct vm_space *space;
    struct vnode *root;

    bus_interrupts_off();

    proc = current_proc;

    space = sched_curr_thread->address_space;
    root = proc->root;

    if (vfs_open(proc, &exe, path, O_RDONLY) != 0) {
        return -(ENOENT);
    }

    FOP_SEEK(exe, 0, SEEK_END);

    size = FILE_POSITION(exe);

    FOP_SEEK(exe, 0, SEEK_SET);

    elf = calloc(1, size);

    FOP_READ(exe, (char*)elf, size);

    cp_argv = copy_strings(argv, &argv_size, &argc);
    cp_envp = copy_strings(envp, &envp_size, &envc);

    unload_userspace(space);

    elf_get_dimensions(elf, &prog_low, &prog_high);

    prog_size = prog_high - prog_low;

    vm_map(space, (void*)prog_low, prog_size, VM_EXEC | VM_READ | VM_WRITE);

    memset((void*)prog_low, 0, prog_size);

    elf_load_image(space, elf);
    elf_zero_sections(space, elf);

    file_close(exe);

    proc->base = prog_low;
    proc->brk = prog_high;
    proc->root = root;
    proc->thread = sched_curr_thread;

    list_append(&proc->threads, sched_curr_thread);

    sched_curr_thread->u_stack_top = KERNEL_VIRTUAL_BASE - 1;

    for (stack_bottom = 0xBFFFF000; stack_bottom > 0xBFFFA000; stack_bottom -= 0x1000) {
        vm_map(space, (void*)stack_bottom, 0x1000, VM_READ | VM_WRITE);
        memset((void*)stack_bottom, 0, 0x1000);
    }

    sched_curr_thread->u_stack_bottom = stack_bottom + 0x1000;

    /*
     * This is kind of messy, but this is pushing argv and envp onto the stack so it can be accessed from userland
     */
    stackp = (uintptr_t)0xBFFFFFFF;
    stackp -= envp_size;
    memcpy((void*)stackp, cp_envp, envp_size);
    u_envp = stackp;
    realign_pointers((uintptr_t*)stackp, (uintptr_t)cp_envp - stackp);
    stackp -= argv_size;
    memcpy((void*)stackp, cp_argv, argv_size);
    u_argv = stackp;
    realign_pointers((uintptr_t*)stackp, (uintptr_t)cp_argv - stackp);

    *((uint32_t*)(stackp - 4)) = u_envp;
    *((uint32_t*)(stackp - 8)) = envc;
    *((uint32_t*)(stackp - 12)) = u_argv;
    *((uint32_t*)(stackp - 16)) = argc;

    stackp -= 20;

    strncpy(proc->name, cp_argv[0], 256);

    free(cp_argv);
    free(cp_envp);

    // now just make sure we close any private files

    for (i = 0; i < 4096; i++) {
        struct file *fp;

        fp = proc->files[i];

        if (fp && (fp->flags & O_CLOEXEC)) {
            proc->files[i] = NULL;
            file_close(fp);
        }
    }

    entrypoint = elf->e_entry;

    free(elf);

    bus_interrupts_on();
    return_to_usermode(entrypoint, stackp, stackp, 0);

    return 0;
}

