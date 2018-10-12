#include <rtl/list.h>
#include <rtl/malloc.h>
#include <rtl/string.h>
#include <rtl/types.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vm.h>
#include <sys/i686/elf32.h>
// remove
#include <rtl/printf.h>

static const char **
copy_strings(const char **strs, size_t *sizep, int *countp)
{
    int count = 0;
    size_t size = 0;
    
    const char **iter = strs;

    for (count = 0; *iter; iter++, count++) {
        size += strlen(*iter) + 1;
    }

    size_t ptrs_size = (count + 1) * sizeof(uintptr_t);
    void *buf = calloc(0, size + ptrs_size);
    uintptr_t *str_ptrs = (uint32_t*)buf;
    
    iter = strs;

    char *dest = (char*)((uintptr_t)buf + ptrs_size);
    
    for (int i = 0; *iter; iter++, i++) {
        const char *src = *iter;
        strcpy(dest, src);
        str_ptrs[i] = (uintptr_t)dest;
        dest += strlen(src) + 1;
    }

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
elf_load_image(struct vm_space *space, struct elf32_ehdr *hdr, uintptr_t *vlow, uintptr_t *vhigh)
{
    struct elf32_phdr *phdrs = elf_get_pheader(hdr);

    uintptr_t high = 0;
    uintptr_t low = 0xFFFFFFFF;

    for (int i = 0; i < hdr->e_phnum; i++) {
        struct elf32_phdr *phdr = &phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            void *vaddr = vm_map(space, (void*)phdr->p_vaddr, phdr->p_memsz, PROT_EXEC | PROT_READ | PROT_WRITE);

            memcpy(vaddr, ((void*)(uintptr_t)hdr + phdr->p_offset), phdr->p_filesz);
            
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
elf_zero_sections(struct vm_space *space, struct elf32_ehdr *hdr)
{
    struct elf32_shdr *shdr = elf_get_sheader(hdr);

    for (int i = 0; i < hdr->e_shnum; i++) {
        struct elf32_shdr *section = &shdr[i];

        if (section->sh_type == SHT_NOBITS) {

            if (!section->sh_size) {
                continue;
            }

            if (section->sh_flags & SHF_ALLOC) {
                memset((void*)section->sh_addr, 0, section->sh_size);
            }
        }
    }
}

static void
unload_userspace(struct vm_space *space)
{
    list_iter_t iter;

    list_get_iter(&space->map, &iter);

    struct vm_block *block;

    while (iter_move_next(&iter, (void**)&block)) {

        if (!(block->prot & PROT_KERN)) {
            vm_unmap(space, (void*)block->start_virtual, block->size);
        }
    }

    iter_close(&iter);
}

int
proc_execve(const char *path, const char **argv, const char **envp)
{
    /* defined in sys/i686/sched.c */
    extern struct thread *sched_curr_thread;

    struct vm_space *space = sched_curr_thread->address_space;
    struct proc *proc = sched_curr_thread->proc;
    struct vfs_node *root = proc->root;

    struct file *exe;
    printf("root is %x\n", root);

    if (vfs_open(root, &exe, path, O_RDONLY) == 0) {
        printf("open!\n");
        vfs_seek(exe, 0, SEEK_END);

        size_t size = vfs_tell(exe);

        vfs_seek(exe, 0, SEEK_SET);

        struct elf32_ehdr *elf = (struct elf32_ehdr*)malloc(size);

        vfs_read(exe, (char*)elf, size);

        int argc;
        int envc;
        size_t argv_size;
        size_t envp_size;

        const char **cp_argv = copy_strings(argv, &argv_size, &argc);
        const char **cp_envp = copy_strings(argv, &envp_size, &envc);

        unload_userspace(space);

        uintptr_t prog_low;
        uintptr_t prog_high;

        elf_load_image(space, elf, &prog_low, &prog_high);
        elf_zero_sections(space, elf);

        vfs_close(exe);

        /* defined in sys/i686/kern/sched.c */
        extern struct thread *sched_curr_thread;

        /* defined in sys/i686/kern/usermode.asm */
        extern void return_to_usermode(uintptr_t target, uintptr_t stack);

        if (sched_curr_thread && sched_curr_thread->proc) {
            //proc_destroy(sched_curr_thread->proc);
        }

        struct proc *proc = proc_new();

        proc->base = prog_low;
        proc->brk = prog_high;
        proc->root = root;
        proc->thread = sched_curr_thread;

        list_append(&proc->threads, sched_curr_thread);

        sched_curr_thread->proc = proc;
        
        vm_map(space, (void*)0xBFFFFFFF, 1, PROT_READ | PROT_WRITE);

        sched_curr_thread->u_stack_bottom = 0xBFFFF000;
        sched_curr_thread->u_stack_top = 0xC0000000;

        /*
         * This is kind of messy, but this is pushing argv and envp onto the stack so it can be accessed from userland
         */
        uintptr_t stackp = (uintptr_t)0xBFFFFFFF;
        stackp -= envp_size;
        memcpy((void*)stackp, cp_envp, envp_size);
        uintptr_t u_envp = stackp;
        realign_pointers((uintptr_t*)stackp, (uintptr_t)cp_envp - stackp);
        stackp -= argv_size;
        memcpy((void*)stackp, cp_argv, argv_size);
        uintptr_t u_argv = stackp;
        realign_pointers((uintptr_t*)stackp, (uintptr_t)cp_argv - stackp);
        *((uint32_t*)(stackp - 4)) = u_envp;
        *((uint32_t*)(stackp - 8)) = envc;
        *((uint32_t*)(stackp - 12)) = u_argv;
        *((uint32_t*)(stackp - 16)) = argc;
        stackp -= 20;

        free(cp_argv);
        free(cp_envp);
        printf("Preparing to jump to %x\n", stackp);
        return_to_usermode(elf->e_entry, stackp);
    }

    return 0;
}
