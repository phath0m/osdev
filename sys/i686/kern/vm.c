/*
 * sys/i686/kern/vm.c
 * Responsible for managing virtual memory on i686 compatible devices
 */

#include <ds/list.h>
#include <machine/multiboot.h>
#include <machine/vm.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vm.h>

struct frame {
    uintptr_t   addr;
    uint32_t    ref_count;
};

struct list pages_allocated;
struct list pages_free;
uintptr_t   pages_physical_brk;

/*
 * track allocations for kstat
 */
int frame_count = 0;
int page_table_count = 0;
int vm_block_count = 0;
int vm_space_count = 0;

static struct frame *
page_find_free_block()
{
    struct frame *result = NULL;
    
    if (list_remove_front(&pages_free, (void**)&result)) {
        return result;
    }

    return NULL;
}

static uintptr_t
frame_alloc(struct frame **res)
{
    struct frame *block = page_find_free_block();

    if (!block) {
        block = (struct frame*)calloc(1, sizeof(struct frame));
        block->addr = pages_physical_brk;
        pages_physical_brk += PAGE_SIZE;
        frame_count++;
    }

    list_append(&pages_allocated, block);

    *res = block;

    return block->addr;
}

static void
frame_free(void *addr)
{
    list_iter_t iter;

    list_get_iter(&pages_allocated, &iter);

    struct frame *block = NULL;
    struct frame *to_remove = NULL;

    while (iter_move_next(&iter, (void**)&block)) {
        if (block->addr == (uintptr_t)addr) {
            to_remove = block;
            break;
        }
    }
    
    iter_close(&iter);

    if (to_remove) {
        frame_count--;
        list_remove(&pages_allocated, to_remove);
        free(to_remove);
    }
}

static void
page_map_entry(struct page_directory *directory, uintptr_t vaddr, uintptr_t paddr, bool write, bool user)
{
    uint32_t page_table = vaddr / PAGE_SIZE;
    uint32_t page_table_entry = page_table / 1024;

    struct page_directory_entry *dir_entry = &directory->tables[page_table_entry];
    struct page_table *table = (struct page_table*)(dir_entry->address << 12);


    if (!dir_entry->present) {    
        table = (struct page_table*)malloc_pa(sizeof(struct page_table));
        page_table_count++;
        memset(table, 0, sizeof(struct page_table));

        dir_entry->present = 1;
        dir_entry->read_write = 1;
        dir_entry->user = 1;
        dir_entry->address = (((uint32_t)table) - KERNEL_VIRTUAL_BASE) >> 12;
     }
     else if (dir_entry->size) {
         panic("cannot re-map huge page!");
    } else {
        table = (struct page_table*)((uintptr_t)table + KERNEL_VIRTUAL_BASE);
    }

    struct page_entry *page = &table->pages[page_table % 1024];

    page->frame = paddr >> 12;
    page->present = true;
    page->read_write = write;
    page->user = 1;
    
    asm volatile("invlpg (%0)" : : "b"(vaddr) : "memory");
}

static void
page_directory_free(struct page_directory *directory)
{
    for (int i = 0; i < 1024; i++) {
        struct page_directory_entry *entry = &directory->tables[i];

        if (entry->present && !entry->size) {
            struct page_table *table = (struct page_table*)((entry->address << 12) + KERNEL_VIRTUAL_BASE);
            page_table_count--;
            entry->present = false;
            free(table);
        }
    }

    vm_space_count--;
    free(directory);
}

struct vm_block *
vm_block_new()
{
    struct vm_block *block = (struct vm_block*)calloc(1, sizeof(struct vm_block));
    vm_block_count++;
    return block;
}

void *
vm_share(struct vm_space *space1, struct vm_space *space2, void *addr1, void *addr2, size_t length, int prot)
{
    int required_pages = PAGE_COUNT(length);

    bool write = VM_IS_WRITABLE(prot);
    bool user = VM_IS_USER(prot);

    if (user) {
        addr1 = (void*)va_alloc_block(space1->uva_map, (uintptr_t)addr1, length);
    } else {
        addr1 = (void*)va_alloc_block(space1->kva_map, (uintptr_t)addr1, length);
    }

    uint32_t offset = (uint32_t)addr2 & 0x0000FFF;
    
    addr1 = (void*)ALIGN_ADDRESS(addr1);
    addr2 = (void*)ALIGN_ADDRESS(addr2);

    struct page_directory *directory = (struct page_directory*)space1->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *src_block = vm_find_block(space2, (uintptr_t)addr2 + (i * PAGE_SIZE));
        struct vm_block *block = vm_block_new();
        struct frame *frame = (struct frame*)src_block->state;

        uintptr_t paddr = src_block->start_physical;
        uintptr_t vaddr = (uintptr_t)addr1 + (PAGE_SIZE * i);

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

void *
vm_map(struct vm_space *space, void *addr, size_t length, int prot)
{
    int required_pages = PAGE_COUNT(length);

    bool write = VM_IS_WRITABLE(prot);
    bool user = VM_IS_USER(prot);

    if (user) {
        addr = (void*)va_alloc_block(space->uva_map, (uintptr_t)addr, length);
    } else {
        addr = (void*)va_alloc_block(space->kva_map, (uintptr_t)addr, length);
    }

    struct page_directory *directory = (struct page_directory*)space->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct frame *frame;

        struct vm_block *block = vm_block_new();

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

void *
vm_map_physical(struct vm_space *space, void *addr, uintptr_t physical, size_t length, int prot)
{
    int required_pages = PAGE_COUNT(length);

    bool write = VM_IS_WRITABLE(prot);
    bool user = VM_IS_USER(prot);

    if (user) {
        addr = (void*)va_alloc_block(space->uva_map, (uintptr_t)addr, length);
    } else {
        addr = (void*)va_alloc_block(space->kva_map, (uintptr_t)addr, length);
    }

    struct page_directory *directory = (struct page_directory*)space->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *block = vm_block_new();

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

void
vm_unmap(struct vm_space *space, void *addr, size_t length)
{
    int required_pages = PAGE_COUNT(length);

    addr = (void*)ALIGN_ADDRESS(addr);

    if ((uintptr_t)addr + length < 0xC0000000) {
        va_free_block(space->uva_map, (uintptr_t)addr, length);
    } else if ((uintptr_t)addr >= space->kernel_brk && ((uintptr_t)addr < space->kernel_end)) {
        va_free_block(space->kva_map, (uintptr_t)addr, length);
    }

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *block = vm_find_block(space, (uintptr_t)addr + (i * PAGE_SIZE));
        struct frame *frame = (struct frame*)block->state;

        if (frame) {
            frame->ref_count--;
        }

        if (frame && frame->ref_count <= 0) {
            frame_free((void*)block->start_physical);
        }

        list_remove(&space->map, block);
        
        free(block);
        vm_block_count--;
    }
}

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
}

static void
map_vbe_data(struct page_directory *directory)
{
    /* defined in sys/i686/kern/premain.c */
    extern multiboot_info_t *multiboot_header;

    if (!multiboot_header->vbe_mode_info) {
        return;
    }

    /*
     * so this is really a hack in that it violates my own principles of separating shit
     * but honestly I don't know a better place to put this code. we need to make sure that
     * the linear framebuffer used for VBE is properly mapped for each vm_space we create
     * to ensure we can seemlessly access the framebuffer
     */
    vbe_info_t *info = (vbe_info_t*)(KERNEL_VIRTUAL_BASE + multiboot_header->vbe_mode_info);    

    for (uintptr_t i = 0; i < info->Yres * info->pitch; i += 0x1000) {
        page_map_entry(directory, info->physbase + i, info->physbase + i, true, false);
    } 
}

struct vm_space *
vm_space_new()
{
    struct vm_space *vm_space = (struct vm_space*)calloc(0, sizeof(struct vm_space));
    struct page_directory *directory = (struct page_directory*)malloc_pa(sizeof(struct page_directory));
    vm_space_count++;

    memset(directory, 0, sizeof(struct page_directory));

    
    uint32_t page_start = KERNEL_VIRTUAL_BASE >> 22;

    for (int i = 0; i < 127; i++) {
        struct page_directory_entry *dir_entry = &directory->tables[page_start + i];
        dir_entry->present = true;
        dir_entry->read_write = true;
        dir_entry->user = false;
        dir_entry->size = true; // huge page
        dir_entry->address = (i * 0x400000) >> 12;
    }

    map_vbe_data(directory);

    vm_space->uva_map = va_map_new(0, KERNEL_VIRTUAL_BASE);
    vm_space->kva_map = va_map_new(KERNEL_VIRTUAL_BASE + 0x1FC00000, 0xE0000000);
    vm_space->kernel_brk = KERNEL_VIRTUAL_BASE + 0x1FC00000;
    vm_space->kernel_end = 0xE0000000;
    vm_space->state_physical = (void*)((uint32_t)directory - KERNEL_VIRTUAL_BASE);
    vm_space->state_virtual = (void*)directory;

    return vm_space;
}

__attribute__((constructor))
void
_init_vm()
{
    /* defined in sys/rtl/malloc.c */
    extern uintptr_t kernel_heap_end;

    pages_physical_brk = kernel_heap_end + PAGE_SIZE;
    pages_physical_brk -= KERNEL_VIRTUAL_BASE;
    pages_physical_brk &= 0xFFFFF000;
}


