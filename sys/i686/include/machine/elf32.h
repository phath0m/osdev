#ifndef SYS_I686_ELF32_H
#define SYS_I686_ELF32_H

#include <sys/string.h>
#include <sys/types.h>

#define ELF_NIDENT                      16
#define ELFMAG0                         0x7F
#define ELFMAG1                         'E'
#define ELFMAG2                         'L'
#define ELFMAG3                         'F'
#define ELFDATA2LSB                     (1)
#define ELFCLASS32                      (1)
#define EM_386                          (3)
#define EV_CURRENT                      (1)
#define ELF32_ST_BIND(INFO)     ((INFO) >> 4)
#define ELF32_ST_TYPE(INFO)     ((INFO) & 0x0F)
#define ELF32_R_SYM(INFO)       ((INFO) >> 8)
#define ELF32_R_TYPE(INFO)      ((uint8_t)(INFO))
#define SHN_UNDEF                       (0x00)
#define DO_386_32(S, A) ((S) + (A))
#define DO_386_PC32(S, A, P)    ((S) + (A)-(P))
#define SHN_ABS                 0xfff1

struct elf32_ehdr {
    uint8_t     e_ident[ELF_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
};

struct elf32_sym {
    uint32_t    st_name;
    uint32_t    st_value;
    uint32_t    st_size;
    uint8_t     st_info;
    uint8_t     st_other;
    uint16_t    st_shndx;
};

struct elf32_shdr {
    uint32_t    sh_name;
    uint32_t    sh_type;
    uint32_t    sh_flags;
    uint32_t    sh_addr;
    uint32_t    sh_offset;
    uint32_t    sh_size;
    uint32_t    sh_link;
    uint32_t    sh_info;
    uint32_t    sh_addralign;
    uint32_t    sh_entsize;
};

struct elf32_phdr {
	uint32_t	p_type;
	uint32_t 	p_offset;
	uint32_t	p_vaddr;
	uint32_t	p_paddr;
	uint32_t	p_filesz;
	uint32_t	p_memsz;
	uint32_t	p_flags;
	uint32_t	p_align;
};

struct elf32_rel {
    uint32_t    r_offset;
    uint32_t    r_info;
};

struct elf32_rela {
    uint32_t    r_offset;
    uint32_t    r_info;
    int32_t     r_addend;
};

enum STT_BINDINGS {
    STB_LOCAL               = 0,    // Local scope
    STB_GLOBAL              = 1,    // Global scope
    STB_WEAK                = 2     // Weak, (ie. __attribute__((weak)))
};

enum PT_TYPES {
    PT_NULL         = 0,
    PT_LOAD         = 1,
    PT_DYNAMIC      = 2,
    PT_INTERP       = 3,
    PT_NOTE         = 4,
    PT_SHLIB        = 5,
};

enum SHT_TYPES {
    SHT_NULL        = 0,    // Null section
    SHT_PROGBITS    = 1,    // Program information
    SHT_SYMTAB      = 2,    // Symbol table
    SHT_STRTAB      = 3,    // String table
    SHT_RELA        = 4,    // Relocation (w/ addend)
    SHT_NOBITS      = 8,    // Not present in file
    SHT_REL         = 9,    // Relocation (no addend)
};

enum STT_TYPES {
    STT_NOTYPE              = 0,    // No type
    STT_OBJECT              = 1,    // Variables, arrays, etc.
    STT_FUNC                = 2     // Methods or functions
};

enum SHT_ATTRIBUTES {
    SHF_WRITE       = 0x01, // Writable section
    SHF_ALLOC       = 0x02  // Exists in memory
};

enum ELF_IDENT {
    EI_MAG0         = 0,    // 0x7F
    EI_MAG1         = 1,    // 'E'
    EI_MAG2         = 2,    // 'L'
    EI_MAG3         = 3,    // 'F'
    EI_CLASS        = 4,    // Architecture (32/64)
    EI_DATA         = 5,    // Byte Order
    EI_VERSION      = 6,    // ELF Version
    EI_OSABI        = 7,    // OS Specific
    EI_ABIVERSION   = 8,    // OS Specific
    EI_PAD          = 9     // Padding
};

enum ELF_TYPE {
    ET_NONE         = 0,    // Unkown Type
    ET_REL          = 1,    // Relocatable File
    ET_EXEC         = 2     // Executable File
};

enum RTT_TYPES {
    R_386_NONE              = 0,
    R_386_32                = 1,
    R_386_PC32              = 2
};

static inline struct elf32_shdr *elf_get_sheader(struct elf32_ehdr *hdr) {
    return (struct elf32_shdr *)((int)hdr + hdr->e_shoff);
}

static inline struct elf32_phdr *
elf_get_pheader(struct elf32_ehdr *hdr)
{
    return (struct elf32_phdr*)((uintptr_t)hdr + hdr->e_phoff);
}

static inline struct elf32_shdr *elf_section(struct elf32_ehdr *hdr, int idx) {
    return &elf_get_sheader(hdr)[idx];
}

static inline char *elf_str_table(struct elf32_ehdr *hdr) {
    if (hdr->e_shstrndx == SHN_UNDEF)
        return NULL;
    return (char *)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}

static inline char *elf_lookup_string(struct elf32_ehdr *hdr, int offset)
{
    char *strtab = elf_str_table(hdr);

    if (!strtab)
        return NULL;
    return strtab + offset;
}

static inline struct elf32_shdr *elf_section_s(struct elf32_ehdr *hdr, const char *str)
{
    for (int i = 0; i < hdr->e_shnum; i++) {
        struct elf32_shdr *sec = elf_section(hdr, i);

        if (!strcmp(elf_lookup_string(hdr, sec->sh_name), ".strtab")) {
            return sec;
        }

    }
    return NULL;
}

static inline char *elf_lookup_string_strtab(struct elf32_ehdr *hdr, int offset) {
    char *strtab = (char *)hdr + elf_section_s(hdr, ".strtab")->sh_offset;

    if (!strtab) {
        return NULL;
    }
    return strtab + offset;
}

#endif
