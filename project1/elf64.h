/*
 * file:        elf64.h
 * description: easier-to-use version of the 64-bit parts of elf.h
 * license:     GPL 2.1 or higher, derived from the GNU C Library
 */

#ifndef __ELF64_H__
#define __ELF64_H__

/* file type */
enum ftype {
    ET_NONE = 0,
    ET_REL = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4,
    ET_16 = 0xFFFF
} __attribute__((__packed__));

/* machine type - ignore all the others */
enum mtype {
    EM_386 = 3,
    EM_16 = 0xFFFF
} __attribute__((__packed__));

/* ELF only has one version */
enum vrsion {
    EV_CURRENT = 1
};

/* class and data encoding are bytes 4 and 5 of e_ident */
enum ei {
    EI_CLASS = 4, /* File class byte index */
    EI_DATA = 5   /* Data encoding byte index */
};

enum eiclass {
    ELFCLASSNONE = 0, /* Invalid class */
    ELFCLASS32 = 1,   /* 32-bit objects */
    ELFCLASS64 = 2,   /* 64-bit objects */
    ELFCLASSNUM = 3
};

enum eidata {
    ELFDATANONE = 0, /* Invalid data encoding */
    ELFDATA2LSB = 1, /* 2's complement, little endian */
    ELFDATA2MSB = 2  /* 2's complement, big endian */
};

/* so we don't need <stdint.h> */
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/* the ELF header itself */
struct elf64_ehdr {
    unsigned char e_ident[16]; /* Magic number and other info */
    enum ftype e_type;         /* Object file type */
    enum mtype e_machine;      /* Architecture */
    uint32_t e_version;        /* Object file version */
    void *e_entry;             /* Entry point virtual address */
    uint64_t e_phoff;          /* Program header table file offset */
    uint64_t e_shoff;          /* Section header table file offset */
    uint32_t e_flags;          /* Processor-specific flags */
    uint16_t e_ehsize;         /* ELF header size in bytes */
    uint16_t e_phentsize;      /* Program header table entry size */
    uint16_t e_phnum;          /* Program header table entry count */
    uint16_t e_shentsize;      /* Section header table entry size */
    uint16_t e_shnum;          /* Section header table entry count */
    uint16_t e_shstrndx;       /* Section header string table index */
} __attribute__((__packed__));

/* program header table */
enum ptype {
    PT_NULL = 0,         /* Program header table entry unused */
    PT_LOAD = 1,         /* Loadable program segment */
    PT_DYNAMIC = 2,      /* Dynamic linking information */
    PT_INTERP = 3,       /* Program interpreter */
    PT_NOTE = 4,         /* Auxiliary information */
    PT_SHLIB = 5,        /* Reserved */
    PT_PHDR = 6,         /* Entry for header table itself */
    PT_TLS = 7,          /* Thread-local storage segment */
    PT_NUM = 8,          /* Number of defined types */
    PT_LOOS = 0x60000000,
    PT_GNU_EH_FRAME = 0x6474e550, /* GCC .eh_frame_hdr segment */
    PT_GNU_STACK = 0x6474e551,    /* Indicates stack executability */
    PT_GNU_RELRO = 0x6474e552,    /* Read-only after relocation */
    PT_LOSUNW = 0x6ffffffa,
    PT_SUNWBSS = 0x6ffffffa, /* Sun Specific segment */
    PT_SUNWSTACK = 0x6ffffffb, /* Stack segment */
    PT_HISUNW = 0x6fffffff,
    PT_HIOS = 0x6fffffff,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff
} __attribute__((__packed__));

enum pflag {
    PF_X = (1 << 0), /* Segment is executable */
    PF_W = (1 << 1), /* Segment is writable */
    PF_R = (1 << 2)  /* Segment is readable */
};

struct elf64_phdr {
    enum ptype p_type;  /* Segment type */
    uint32_t p_flags;   /* Segment flags */
    uint64_t p_offset;  /* Segment file offset */
    void *p_vaddr;      /* Segment virtual address */
    void *p_paddr;      /* Segment physical address */
    uint64_t p_filesz;  /* Segment size in file */
    uint64_t p_memsz;   /* Segment size in memory */
    uint64_t p_align;   /* Segment alignment */
} __attribute__((__packed__));

/* section header table */
enum shtype {
    SHT_NULL = 0,
    SHT_PROGBITS = 1,
    SHT_SYMTAB = 2,
    SHT_STRTAB = 3,
    SHT_RELA = 4,
    SHT_HASH = 5,
    SHT_DYNAMIC = 6,
    SHT_NOTE = 7,
    SHT_NOBITS = 8,
    SHT_REL = 9,
    SHT_SHLIB = 10,
    SHT_DYNSYM = 11,
    SHT_INIT_ARRAY = 14,
    SHT_FINI_ARRAY = 15,
    SHT_PREINIT_ARRAY = 16,
    SHT_GROUP = 17,
    SHT_SYMTAB_SHNDX = 18,
    SHT_32 = 0xFFFFFFFF
} __attribute__((__packed__));

enum shflag {
    SHF_WRITE = (1 << 0),
    SHF_ALLOC = (1 << 1),
    SHF_EXECINSTR = (1 << 2),
    SHF_MERGE = (1 << 4),
    SHF_STRINGS = (1 << 5),
    SHF_INFO_LINK = (1 << 6),
    SHF_LINK_ORDER = (1 << 7),
    SHF_GROUP = (1 << 9),
    SHF_TLS = (1 << 10)
};

struct elf64_section {
    uint32_t sh_name;
    enum shtype sh_type;
    uint64_t sh_flags;
    void *sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} __attribute__((__packed__));

/* symbol table structure */
struct elf64_sym {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    void *st_value;
    uint64_t st_size;
} __attribute__((__packed__));

#endif /* __ELF64_H__ */
