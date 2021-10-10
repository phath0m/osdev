/*
 * vm.c - Machine dependent virtual memory manager 
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
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <machine/vm_private.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vm.h>

/* frame structure; a single block of physical memory that corresponds to a page */
struct frame {
    uintptr_t   addr;       /* physical address */
    uint32_t    ref_count;  /* how many virtual addresses refer to this physical address? */
};

struct list frames_allocated;       /* list of frames that have been allocated */
struct list frames_free;            /* lists of frames that are free and available for re-use */
uintptr_t   kernel_physical_brk;    /* physical memory highwater mark */

struct pool         page_table_pool;
struct pool         page_directory_pool;

static spinlock_t   frame_alloc_lock;

/* global statistics for VM structures allocated in kernel space */
struct vm_statistics vm_stat;

/* find a recycled frame */
static struct frame *
get_free_frame()
{
    struct frame *result;
    
    if (list_remove_front(&frames_free, (void**)&result)) {
        return result;
    }

    return NULL;
}

/* allocates a new block of physical memory  */
static uintptr_t
frame_alloc(struct frame **res)
{
    struct frame *frame;

    spinlock_lock(&frame_alloc_lock);

    frame = get_free_frame();

    if (!frame) {
        frame = (struct frame*)calloc(1, sizeof(struct frame));
        frame->addr = kernel_physical_brk;
        kernel_physical_brk += PAGE_SIZE;
        VMSTAT_INC_FRAME_COUNT(&vm_stat); 
    }

    list_append(&frames_allocated, frame);

    spinlock_unlock(&frame_alloc_lock);

    *res = frame;

    return frame->addr;
}

/* marks a block of physical memory as free */
static void
frame_free(void *addr)
{
    list_iter_t iter;

    struct frame *frame;
    struct frame *to_remove;

    spinlock_lock(&frame_alloc_lock);

    list_get_iter(&frames_allocated, &iter);

    to_remove = NULL;

    while (iter_move_next(&iter, (void**)&frame)) {
        if (frame->addr == (uintptr_t)addr) {
            to_remove = frame;
            break;
        }
    }
    
    iter_close(&iter);

    if (to_remove) {
        VMSTAT_DEC_FRAME_COUNT(&vm_stat);
        list_remove(&frames_allocated, to_remove);
        free(to_remove);
    }

    spinlock_unlock(&frame_alloc_lock);
}

/* actually map a virtual address to a physical address  */
static void
page_map_entry(struct page_directory *directory, uintptr_t vaddr, uintptr_t paddr, bool write, bool user)
{
    uint32_t page_table;
    uint32_t page_table_entry;

    struct page_directory_entry *dir_entry;
    struct page_entry *page;
    struct page_table *table;

    page_table = PAGE_INDEX(vaddr);
    page_table_entry = page_table / 1024;

    dir_entry = &directory->tables[page_table_entry];
    table = (struct page_table*)(dir_entry->address << 12);

    if (!dir_entry->present) {    
        table = pool_get(&page_table_pool);
        VMSTAT_INC_PAGE_TABLE_COUNT(&vm_stat);
        memset(table, 0, sizeof(struct page_table));

        dir_entry->present = 1;
        dir_entry->read_write = 1;
        dir_entry->user = 1;
        dir_entry->address = FRAME_INDEX(KVATOP(table));
     }
     else if (dir_entry->size) {
         panic("we shouldn't be unmapping huge pages. something is wrong");
    } else {
        table = (struct page_table*)PTOKVA(table);
    }

    page = &table->pages[page_table % 1024];

    page->frame = FRAME_INDEX(paddr);
    page->present = true;
    page->read_write = write;
    page->user = 1;
    
    asm volatile("invlpg (%0)" : : "b"(vaddr) : "memory");
}

/* destroy a page directory structure */
static void
page_directory_free(struct page_directory *directory)
{
    int i;

    for (i = 0; i < 1024; i++) {
        struct page_directory_entry *entry;
        
        entry = &directory->tables[i];

        if (entry->present && !entry->size) {
            struct page_table *table;
            
            table = (struct page_table*)PTOKVA(entry->address << 12);
            VMSTAT_DEC_PAGE_TABLE_COUNT(&vm_stat);
            entry->present = false;
            pool_put(&page_table_pool, table);
        }
    }
   
    pool_put(&page_directory_pool, directory); 
}

/* new page structure */
struct vm_block *
vm_block_new()
{
    struct vm_block *block;
    block = (struct vm_block*)calloc(1, sizeof(struct vm_block));
    VMSTAT_INC_PAGE_COUNT(&vm_stat);
    return block;
}

/* share a virtual address from one address space to another */
void *
vm_share(struct vm_space *space1, struct vm_space *space2, void *addr1, void *addr2, size_t length, int prot)
{
    bool write;
    bool user;
    int i;
    int required_pages;
    uintptr_t offset;
    uintptr_t paddr;
    uintptr_t vaddr;

    struct page_directory *directory;

    required_pages = PAGE_COUNT(length);

    write = VM_IS_WRITABLE(prot);
    user = VM_IS_USER(prot);

    if (user) {
        addr1 = (void*)va_alloc_block(space1->uva_map, (uintptr_t)addr1, length);
    } else {
        addr1 = (void*)va_alloc_block(space1->kva_map, (uintptr_t)addr1, length);
    }

    offset = PAGE_OFFSET(addr2);

    addr1 = (void*)ALIGN_ADDRESS(addr1);
    addr2 = (void*)ALIGN_ADDRESS(addr2);

    directory = (struct page_directory*)space1->state_virtual;

    for (i = 0; i < required_pages; i++) {
        struct vm_block *src_block;
        struct vm_block *block;
        struct frame *frame;

        src_block = vm_find_block(space2, (uintptr_t)addr2 + (i * PAGE_SIZE));
        block = vm_block_new();
        frame = (struct frame*)src_block->state;

        paddr = src_block->start_physical;
        vaddr = (uintptr_t)addr1 + (PAGE_SIZE * i);

        frame->ref_count++;

        block->size = PAGE_SIZE;
        block->start_physical = paddr;
        block->start_virtual = vaddr;
        block->prot = prot;
        block->state = frame;

        list_append(&space1->map, block);

        page_map_entry(directory, vaddr, paddr, write, user);
    }

    return (void*)((uint32_t)addr1 + offset);
}

/* maps a contiguous block of pages to an address space */
void *
vm_map(struct vm_space *space, void *addr, size_t length, int prot)
{
    bool write;
    bool user;
    int i;
    int required_pages;

    struct page_directory *directory;

    required_pages = PAGE_COUNT(length);

    write = VM_IS_WRITABLE(prot);
    user = VM_IS_USER(prot);

    if (user) {
        addr = (void*)va_alloc_block(space->uva_map, (uintptr_t)addr, length);
    } else {
        addr = (void*)va_alloc_block(space->kva_map, (uintptr_t)addr, length);
    }

    directory = (struct page_directory*)space->state_virtual;

    for (i = 0; i < required_pages; i++) {
        struct frame *frame;

        struct vm_block *block;
        
        block = vm_block_new();

        block->size = PAGE_SIZE;
        block->start_physical = frame_alloc(&frame);
        block->start_virtual = ALIGN_ADDRESS((uintptr_t)addr + (PAGE_SIZE * i));
        block->prot = prot;
        block->state = frame;

        frame->ref_count++;

        list_append(&space->map, block);

        page_map_entry(directory, block->start_virtual, block->start_physical, write, user);
    }

    return addr;
}

/* map a physical address to a virtual address */
void *
vm_map_physical(struct vm_space *space, void *addr, uintptr_t physical, size_t length, int prot)
{
    bool user;
    bool write;

    int i;
    int required_pages;

    struct page_directory *directory;

    required_pages = PAGE_COUNT(length);

    write = VM_IS_WRITABLE(prot);
    user = VM_IS_USER(prot);

    if (user) {
        addr = (void*)va_alloc_block(space->uva_map, (uintptr_t)addr, length);
    } else {
        addr = (void*)va_alloc_block(space->kva_map, (uintptr_t)addr, length);
    }

    directory = (struct page_directory*)space->state_virtual;

    for (i = 0; i < required_pages; i++) {
        struct vm_block *block;
        
        block = vm_block_new();

        block->size = PAGE_SIZE;
        block->start_physical = ALIGN_ADDRESS(physical + (PAGE_SIZE * i));
        block->start_virtual = ALIGN_ADDRESS((uintptr_t)addr + (PAGE_SIZE * i));
        block->prot = prot;
        block->state = NULL;

        list_append(&space->map, block);

        page_map_entry(directory, block->start_virtual, block->start_physical, write, user);
    }

    return addr;
}

/* unmaps a contiguous block of pages from an address space */
void
vm_unmap(struct vm_space *space, void *addr, size_t length)
{
    int i;
    int required_pages;
    
    required_pages = PAGE_COUNT(length);

    addr = (void*)ALIGN_ADDRESS(addr);

    if (VA_BOUNDCHECK(space->uva_map, addr, length)) {
        va_free_block(space->uva_map, (uintptr_t)addr, length);
    } else if (VA_BOUNDCHECK(space->kva_map, addr, length)) {
        va_free_block(space->kva_map, (uintptr_t)addr, length);
    } else {
        stacktrace(4);
        panic("attempted to unmap non-mapped address 0x%p\n\r"
            "user base: %p user limit %p\n\r"
            "kernel base: %p kernel limit: %p\n\r",
            addr, space->uva_map->base, space->uva_map->limit, space->kva_map->base, space->kva_map->limit
        );
    }

    for (i = 0; i < required_pages; i++) {
        struct frame *frame;
        struct vm_block *block;
        
        block = vm_find_block(space, (uintptr_t)addr + (i * PAGE_SIZE));
        frame = (struct frame*)block->state;

        if (frame) {
            frame->ref_count--;
        }

        if (frame && frame->ref_count <= 0) {
            frame_free((void*)block->start_physical);
        }

        list_remove(&space->map, block);
        
        free(block);

        VMSTAT_DEC_PAGE_COUNT(&vm_stat);
    }
}

/* destroy an address space */
void
vm_space_destroy(struct vm_space *space)
{
    list_iter_t iter;
    
    struct list to_remove;
    struct vm_block *block;

    memset(&to_remove, 0, sizeof(struct list));

    list_get_iter(&space->map, &iter);

    while (iter_move_next(&iter, (void**)&block)) {
        list_append(&to_remove, block);
    }

    iter_close(&iter);

    list_get_iter(&to_remove, &iter);

    while (iter_move_next(&iter, (void**)&block)) {
        vm_unmap(space, (void*)block->start_virtual, 0x1000);
    }

    iter_close(&iter);

    list_destroy(&to_remove, false);
    list_destroy(&space->map, false);

    page_directory_free((struct page_directory*)space->state_virtual);
    
    free(space->uva_map);
    free(space->kva_map);
    free(space);
    
    VMSTAT_DEC_VM_SPACE_COUNT(&vm_stat);
}

#ifdef USE_BOOTLOADER_GRAPHICS
static void
map_vbe_data(struct page_directory *directory)
{
    /* defined in sys/i686/kern/premain.c */
    extern multiboot_info_t *multiboot_header;

    uintptr_t i;
    vbe_info_t *info;


    if (!multiboot_header->vbe_mode_info) {
        return;
    }

    /*
     * so this is really a hack in that it violates my own principles of separating shit
     * but honestly I don't know a better place to put this code. we need to make sure that
     * the linear framebuffer used for VBE is properly mapped for each vm_space we create
     * to ensure we can seemlessly access the framebuffer
     */
    info = (vbe_info_t*)(KERNEL_VIRTUAL_BASE + multiboot_header->vbe_mode_info);    

    for (i = 0; i < info->Yres * info->pitch; i += 0x1000) {
        page_map_entry(directory, info->physbase + i, info->physbase + i, true, false);
    } 
}
#endif

/* create a new address space */
struct vm_space *
vm_space_new()
{
    int i;
    uint32_t page_start;

    struct vm_space *vm_space;
    struct page_directory *directory;

    vm_space = calloc(1, sizeof(struct vm_space));
    directory = pool_get(&page_directory_pool);

    VMSTAT_INC_VM_SPACE_COUNT(&vm_stat);

    memset(directory, 0, sizeof(struct page_directory));
    
    page_start = KERNEL_VIRTUAL_BASE >> 22;

    for (i = 0; i < 127; i++) {
        struct page_directory_entry *dir_entry;
        
        dir_entry = &directory->tables[page_start + i];
        dir_entry->present = true;
        dir_entry->read_write = true;
        dir_entry->user = false;
        dir_entry->size = true; // huge page
        dir_entry->address = (i * 0x400000) >> 12;
    }

#ifdef USE_BOOTLOADER_GRAPHICS
    map_vbe_data(directory);
#endif

    vm_space->uva_map = va_map_new(0, KERNEL_VIRTUAL_BASE);
    vm_space->kva_map = va_map_new(KERNEL_VIRTUAL_BASE + 0x1FC00000, 0xEA000000);
    vm_space->state_physical = (void*)KVATOP(directory);
    vm_space->state_virtual = (void*)directory;

    return vm_space;
}

void
vm_init()
{
    /* defined in sys/rtl/malloc.c */
    extern uintptr_t kernel_heap_end;

    kernel_physical_brk = kernel_heap_end + PAGE_SIZE;
    kernel_physical_brk -= KERNEL_VIRTUAL_BASE;
    kernel_physical_brk &= 0xFFFFF000;
    
    pool_init(&page_table_pool, sizeof(struct page_table), 4096);
    pool_init(&page_directory_pool, sizeof(struct page_directory), 4096);
}