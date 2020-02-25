/*
 * sys/i686/kern/interrupt.c
 * Enables everything required for hardware and software interrupts as well as context switching
 */
#include <machine/reg.h>
#include <sys/string.h>
#include <sys/types.h>
#include <sys/i686/portio.h>

struct dt_ptr {
    uint16_t    limit;
    uint32_t    base;
} __attribute__((packed));

struct gdt_entry {
    uint16_t    limit_low;
    uint16_t    base_low;
    uint8_t     base_middle;
    uint8_t     access;
    uint8_t     granularity;
    uint8_t     base_high;
} __attribute__((packed));

struct idt_entry {
    uint16_t    offset_1;
    uint16_t    selector;
    uint8_t     zero;
    uint8_t     type_attr;
    uint16_t    offset_2;
} __attribute__((packed));

struct tss_entry {
    uint32_t    prev_tss;
    uint32_t    esp0;
    uint32_t    ss0;
    uint32_t    esp1;
    uint32_t    ss1;
    uint32_t    esp2;
    uint32_t    ss2;
    uint32_t    cr3;
    uint32_t    eip;
    uint32_t    eflags;
    uint32_t    eax;
    uint32_t    ecx;
    uint32_t    edx;
    uint32_t    ebx;
    uint32_t    esp;
    uint32_t    ebp;
    uint32_t    esi;
    uint32_t    edi;
    uint32_t    es;
    uint32_t    cs;
    uint32_t    ss;
    uint32_t    ds;
    uint32_t    fs;
    uint32_t    gs;
    uint32_t    ldt;
    uint16_t    trap;
    uint16_t    iomap_base;
} __attribute__((packed));


struct gdt_entry global_descriptor_table[6];
struct idt_entry interrupt_table[256];
struct tss_entry task_state_segment;

void
gdt_set_gate(uint8_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    struct gdt_entry *entry = &global_descriptor_table[num];

    entry->base_low = (base & 0xFFFF);
    entry->base_middle = (base >> 16) & 0xFF;
    entry->base_high = (base >> 24) & 0xFF;
    entry->limit_low = (limit & 0xFFFF);
    entry->granularity = (limit >> 16) & 0x0F;
    entry->granularity |= granularity & 0xF0;
    entry->access = access;
}

void
idt_set_gate(uint8_t inum, uint32_t base, uint16_t selector, uint8_t flags)
{
    struct idt_entry *entry = &interrupt_table[inum];

    entry->offset_1 = base & 0xFFFF;
    entry->offset_2 = (base >> 16) & 0xFFFF;
    entry->selector = selector;
    entry->zero = 0;
    entry->type_attr = flags;
}

void
set_tss_esp0(uint32_t esp0)
{
    task_state_segment.esp0 = esp0;
}

void
set_task_segment(uint32_t num, uint16_t ss0, uint32_t esp0)
{
    uint32_t base = (uint32_t)&task_state_segment;
    uint32_t limit = base + sizeof(struct tss_entry);

    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&task_state_segment, 0, sizeof(struct tss_entry));

    task_state_segment.ss0 = ss0;
    task_state_segment.esp0 = esp0;

    task_state_segment.cs = 0x0B;
    task_state_segment.ds = 0x13;
    task_state_segment.es = 0x13;
    task_state_segment.fs = 0x13;
    task_state_segment.gs = 0x13;
    task_state_segment.ss = 0x13;
}

__attribute__((constructor))
void
_init_idt()
{
    struct dt_ptr gdt_ptr;
    struct dt_ptr idt_ptr;

    memset(&global_descriptor_table, 0, sizeof(struct gdt_entry) * 6);
    
    gdt_ptr.limit = sizeof(struct gdt_entry) * 6;
    gdt_ptr.base = (uint32_t)&global_descriptor_table;

    gdt_set_gate(0, 0, 0, 0, 0); // null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // usermode code segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // usermode data segment

    set_task_segment(5, 0x10, 0xFFFFFFFF);
    /* defined in sys/i686/kern/gdt.asm */
    
    extern void gdt_flush_ptr(struct dt_ptr *ptr);

    gdt_flush_ptr(&gdt_ptr);

    asm volatile (
        ".Intel_syntax noprefix;"
        "mov ax, 0x2B;"
        "ltr ax;"
        ".att_syntax noprefix;"
    );

    memset(&interrupt_table, 0, sizeof(struct idt_entry) * 256);

    idt_ptr.limit = 2047;
    idt_ptr.base = (uint32_t)&interrupt_table;
    
    io_write_byte(0x20, 0x11);
    io_write_byte(0xA0, 0x11);
    io_write_byte(0x21, 0x20);
    io_write_byte(0xA1, 0x28);
    io_write_byte(0x21, 0x04);
    io_write_byte(0xA1, 0x02);
    io_write_byte(0x21, 0x01);
    io_write_byte(0xA1, 0x01);
    io_write_byte(0x21, 0x00);
    io_write_byte(0xA1, 0x00);


    /*
     * defined in interrupt_handlers.asm
     */
    extern void isr0(struct regs *regs);
    extern void isr1(struct regs *regs);
    extern void isr2(struct regs *regs);
    extern void isr3(struct regs *regs);
    extern void isr4(struct regs *regs);
    extern void isr5(struct regs *regs);
    extern void isr6(struct regs *regs);
    extern void isr7(struct regs *regs);
    extern void isr8(struct regs *regs);
    extern void isr9(struct regs *regs);
    extern void isr10(struct regs *regs);
    extern void isr11(struct regs *regs);
    extern void isr12(struct regs *regs);
    extern void isr13(struct regs *regs);
    extern void isr14(struct regs *regs);
    extern void isr15(struct regs *regs);
    extern void isr16(struct regs *regs);
    extern void isr17(struct regs *regs);
    extern void isr18(struct regs *regs);
    extern void isr19(struct regs *regs);
    extern void isr20(struct regs *regs);
    extern void isr21(struct regs *regs);
    extern void isr22(struct regs *regs);
    extern void isr23(struct regs *regs);
    extern void isr24(struct regs *regs);
    extern void isr25(struct regs *regs);
    extern void isr26(struct regs *regs);
    extern void isr27(struct regs *regs);
    extern void isr28(struct regs *regs);
    extern void isr29(struct regs *regs);
    extern void isr30(struct regs *regs);
    extern void isr31(struct regs *regs);
    
    extern void isr128(struct regs *regs);

    extern void irq0(struct regs *regs);
    extern void irq1(struct regs *regs);
    extern void irq2(struct regs *regs);
    extern void irq3(struct regs *regs);
    extern void irq4(struct regs *regs);
    extern void irq5(struct regs *regs);
    extern void irq6(struct regs *regs);
    extern void irq7(struct regs *regs);
    extern void irq8(struct regs *regs);
    extern void irq9(struct regs *regs);
    extern void irq10(struct regs *regs);
    extern void irq11(struct regs *regs);
    extern void irq12(struct regs *regs);
    extern void irq13(struct regs *regs);
    extern void irq14(struct regs *regs);
    extern void irq15(struct regs *regs);

    uint32_t handler_pointers[] = {
        (uint32_t)isr0, (uint32_t)isr1,
        (uint32_t)isr2, (uint32_t)isr3,
        (uint32_t)isr4, (uint32_t)isr5,
        (uint32_t)isr6, (uint32_t)isr7,
        (uint32_t)isr8, (uint32_t)isr9,
        (uint32_t)isr10, (uint32_t)isr11,
        (uint32_t)isr12, (uint32_t)isr13,
        (uint32_t)isr14, (uint32_t)isr15,
        (uint32_t)isr16, (uint32_t)isr17,
        (uint32_t)isr18, (uint32_t)isr19,
        (uint32_t)isr20, (uint32_t)isr21,
        (uint32_t)isr22, (uint32_t)isr23,
        (uint32_t)isr24, (uint32_t)isr25,
        (uint32_t)isr26, (uint32_t)isr27,
        (uint32_t)isr28, (uint32_t)isr29, 
        (uint32_t)isr30, (uint32_t)isr31,
        (uint32_t)irq0, (uint32_t)irq1,
        (uint32_t)irq2, (uint32_t)irq3,
        (uint32_t)irq4, (uint32_t)irq5,
        (uint32_t)irq6, (uint32_t)irq7,
        (uint32_t)irq8, (uint32_t)irq9,
        (uint32_t)irq10, (uint32_t)irq11,
        (uint32_t)irq12, (uint32_t)irq13,
        (uint32_t)irq14, (uint32_t)irq15,
        NULL
    };

    for (int i = 0; handler_pointers[i]; i++) {
        idt_set_gate(i, handler_pointers[i], 0x08, 0x8E);
    }

    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    asm volatile("lidt (%0)" : : "p"(&idt_ptr));
}
