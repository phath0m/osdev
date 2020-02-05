/*
 * sys/i686/kern/vm.c
 * Responsible for managing virtual memory on i686 compatible devices
 */

#include <ds/list.h>
#include <sys/malloc.h>
#include <sys/string.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/vm.h>
#include <sys/i686/vm.h>
#include <sys/i686/multiboot.h>

#define PHYSICAL_START_ADDR 0x20000000

#define PHYSICAL_START_ADDR 0x20000000
#define BITMAP_INDEX(n) ((n) >> 3)
#define BITMAP_BIT(n) ((n) & 7)
#define BITMAP_SIZE(n) (((n) >> 3) + 1)
#define BITMAP_SET(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] |= (1 << BITMAP_BIT(n)))
#define BITMAP_CLEAR(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] &= ~(1 << BITMAP_BIT(n)))
#define BITMAP_GET(b, n) (((uint8_t*)b)[BITMAP_INDEX(n)] & (1 << BITMAP_BIT(n)))

struct page_block {
    uintptr_t   addr;
    uint32_t    ref_count;
};

struct list pages_allocated;
struct list pages_free;
uintptr_t   pages_physical_brk;

/*
 * track allocations for kstat
 */
int page_block_count = 0;
int page_table_count = 0;
int vm_block_count = 0;
int vm_space_count = 0;

static struct page_block *
page_find_free_block()
{
    struct page_block *result = NULL;
    
    if (list_remove_front(&pages_free, (void**)&result)) {
        return result;
    }

    return NULL;
}

static uintptr_t
page_alloc(struct page_block **res)
{
    struct page_block *block = page_find_free_block();

    if (!block) {
        block = (struct page_block*)calloc(1, sizeof(struct page_block));
        block->addr = pages_physical_brk;
        pages_physical_brk += PAGE_SIZE;
        page_block_count++;
    }

    list_append(&pages_allocated, block);

    *res = block;

    return block->addr;
}

static void
page_free(void *addr)
{
    list_iter_t iter;

    list_get_iter(&pages_allocated, &iter);

    struct page_block *block = NULL;
    struct page_block *to_remove = NULL;

    while (iter_move_next(&iter, (void**)&block)) {
        if (block->addr == (uintptr_t)addr) {
            to_remove = block;
            break;
        }
    }
    
    iter_close(&iter);

    if (to_remove) {
        page_block_count--;
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

    if (page->present) {
        return;
    }
    
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

uintptr_t
vm_find_vsegment(struct vm_space *space, size_t length)
{
    size_t required_pages = ((length - 1) >> 12) + 1;

    int free_page_count = 0;

    for (int i = 1; i < 0xC0000; i++) {

        if (free_page_count == required_pages) {
            return (i - free_page_count) * 4096;
        }

        if (BITMAP_GET(space->va_map, i)) {
            free_page_count = 0;
        } else {
            free_page_count++;
        }
    }

    return 0;
}

void
vm_alloc_vsegment(struct vm_space *space, uintptr_t addr, size_t length)
{
    size_t required_pages = ((length - 1) >> 12) + 1;

    int start_page = addr / 4096;

    for (int i = 0; i < required_pages; i++) {
        BITMAP_SET(space->va_map, start_page + i);
    }
}

void
vm_free_vsegment(struct vm_space *space, uintptr_t addr, size_t length)
{
    size_t required_pages = ((length - 1) >> 12) + 1;

    int start_page = addr / 4096;

    for (int i = 0; i < required_pages; i++) {
        BITMAP_CLEAR(space->va_map, start_page + i);
    }
}

struct vm_block *
vm_find_block(struct vm_space *space, uintptr_t vaddr)
{
    list_iter_t iter;

    list_get_iter(&space->map, &iter);

    struct vm_block *block;
    struct vm_block *res = NULL;

    while (iter_move_next(&iter, (void**)&block)) {
        if (block->start_virtual == vaddr) {
            res = block;
            break;
        }
    }

    iter_close(&iter);

    return res;
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
    int required_pages = ((length - 1) >> 12) + 1;

    bool write = (prot & PROT_WRITE);
    bool user = (prot & PROT_KERN) == 0;

    if (addr1 == NULL && (prot & PROT_KERN)) {
        addr1 = (void*)space1->kernel_brk;
        space1->kernel_brk += (PAGE_SIZE * required_pages);
    } else if (addr1 == NULL) {
        addr1 = (void*)vm_find_vsegment(space1, length);
    }

    if (user) {
        vm_alloc_vsegment(space1, (uintptr_t)addr1, length);
    }

    uint32_t offset = (uint32_t)addr2 & 0x0000FFF;
    
    addr1 = (void*)((uintptr_t)addr1 & 0xFFFFF000);
    addr2 = (void*)((uintptr_t)addr2 & 0xFFFFF000);

    struct page_directory *directory = (struct page_directory*)space1->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *src_block = vm_find_block(space2, (uintptr_t)addr2 + (i * PAGE_SIZE));
        struct vm_block *block = vm_block_new();
        struct page_block *frame = (struct page_block*)src_block->state;

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
    int required_pages = ((length - 1) >> 12) + 1;

    bool write = (prot & PROT_WRITE) != 0;
    bool user = (prot & PROT_KERN) == 0;

    if (addr == NULL && !user) {
        addr = (void*)space->kernel_brk;
        space->kernel_brk += (PAGE_SIZE * required_pages);
    } else if (addr == NULL) {
        addr = (void*)vm_find_vsegment(space, length);
    }

    if (user) {
        vm_alloc_vsegment(space, (uintptr_t)addr, length);
    }

    struct page_directory *directory = (struct page_directory*)space->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct page_block *frame;

        struct vm_block *block = vm_block_new();

        block->size = PAGE_SIZE;
        block->start_physical = page_alloc(&frame);
        block->start_virtual = ((uintptr_t)addr + (PAGE_SIZE * i)) & 0xFFFFF000;
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
    int required_pages = ((length - 1) >> 12) + 1;

    bool write = (prot & PROT_WRITE) != 0;
    bool user = (prot & PROT_KERN) == 0;

    if (addr == NULL && !user) {
        addr = (void*)space->kernel_brk;
        space->kernel_brk += (PAGE_SIZE * required_pages);
    } else if (addr == NULL) {
        addr = (void*)vm_find_vsegment(space, length);
    }

    if (user) {
        vm_alloc_vsegment(space, (uintptr_t)addr, length);
    }

    struct page_directory *directory = (struct page_directory*)space->state_virtual;

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *block = vm_block_new();

        block->size = PAGE_SIZE;
        block->start_physical = (physical + (PAGE_SIZE * i)) & 0xFFFFF000;
        block->start_virtual = ((uintptr_t)addr + (PAGE_SIZE * i)) & 0xFFFFF000;
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
    int required_pages = ((length - 1) >> 12) + 1;
    
    addr = (void*)((uintptr_t)addr & 0xFFFFF000);

    vm_free_vsegment(space, (uintptr_t)addr, length);

    for (int i = 0; i < required_pages; i++) {
        struct vm_block *block = vm_find_block(space, (uintptr_t)addr + (i * PAGE_SIZE));
        struct page_block *frame = (struct page_block*)block->state;

        if (frame) {
            frame->ref_count--;
        }

        if (frame && frame->ref_count <= 0) {
            page_free((void*)block->start_physical);
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

    free(space->va_map);
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

    vm_space->va_map = calloc(1, 0xC0000);
    vm_space->kernel_brk = KERNEL_VIRTUAL_BASE + 0x1FC00000;
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


