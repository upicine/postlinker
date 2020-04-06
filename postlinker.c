#include <elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "postlinker.h"
#include "utils.h"

static const Elf64_Xword sect_flags[] = {
        SHF_ALLOC | SHF_EXECINSTR | SHF_WRITE,
        SHF_ALLOC | SHF_EXECINSTR,
        SHF_ALLOC | SHF_WRITE,
        SHF_ALLOC
};

static const size_t sect_flags_sz = 4;


static Elf64_Word shflag_to_phflag(Elf64_Xword shflag) {
    switch (shflag) {
        case SHF_ALLOC | SHF_EXECINSTR | SHF_WRITE:
            return PF_R + PF_W + PF_X;
        case SHF_ALLOC | SHF_EXECINSTR:
            return PF_R + PF_X;
        case SHF_ALLOC | SHF_WRITE:
            return PF_R + PF_W;
        case SHF_ALLOC:
            return PF_R;
    }
}

off_t calc_alignment(off_t off, uint64_t alignment) {
    if (off % alignment != 0)
        return alignment - (off % alignment);

    return 0;
}


off_t next_available_offset(Elf64_Ehdr *ehdr, Elf64_Phdr* phdr) {
    off_t max_off = 0, file_sz = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (max_off < phdr[i].p_offset) {
            file_sz = phdr[i].p_filesz + calc_alignment(phdr[i].p_filesz, PAGE_SIZE);
            max_off = phdr[i].p_offset;
        }
    }

    return max_off + file_sz;
}


off_t next_available_vaddr(Elf64_Ehdr *ehdr, Elf64_Phdr* phdr) {
    off_t max_vaddr = 0, mem_sz = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (max_vaddr < phdr[i].p_vaddr) {
            mem_sz = phdr[i].p_memsz + calc_alignment(phdr[i].p_memsz, PAGE_SIZE);
            max_vaddr = phdr[i].p_vaddr;
        }
    }

    return max_vaddr + mem_sz;
}


static size_t merge_content(int fd, Elf64_Shdr *shdr, Elf64_Ehdr *ehdr,
                            char** content, const uint64_t flag) {
    size_t content_sz = 0;
    off_t off = 0, addr_align = 0;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_size != 0 && shdr[i].sh_flags == flag) {
            addr_align = calc_alignment(off, shdr[i].sh_addralign);

            content_sz += addr_align + shdr[i].sh_size;
            content = realloc(content, content_sz);
            if (!content) {
                // Tutaj jeszcze chyba jeden check trzeba zrobic, zeby nie bylo use after free
            }

            ssize_t ret = pread(fd, content + off + addr_align, shdr[i].sh_size,
                                shdr[i].sh_offset);
            if (ret < 0) {
                perror("Error creating segments");
                exit(1);
            }

            off += addr_align + shdr[i].sh_size;
        }
    }

    return content_sz;
}

// zwraca liczbe wygenerowanych segmentow
int merge_sections(int fd, Elf64_Shdr* shdr, Elf64_Ehdr* ehdr,
                   Elf64_Phdr* phdr, Elf64_Phdr* new_phdr) {
    char* content = NULL;
    int j = 0;
    off_t off = next_available_offset(ehdr, phdr);
    off_t vaddr = next_available_vaddr(ehdr, phdr);

    for (int i = 0; i < sect_flags_sz; i++) {
        size_t content_sz = merge_content(fd, shdr, ehdr, &content,
                                          sect_flags[i]);
        if (content_sz == 0)
            continue;

        new_phdr[j].p_type = PT_LOAD;
        new_phdr[j].p_flags = shflag_to_phflag(sect_flags[i]);
        new_phdr[j].p_offset = off;
        new_phdr[j].p_memsz = content_sz;
        new_phdr[j].p_filesz = content_sz;
        new_phdr[j].p_vaddr = vaddr;
        new_phdr[j].p_paddr = vaddr;
        new_phdr[j].p_align = PAGE_SIZE;

        ssize_t ret = pwrite(fd, content, content_sz, off);
        if (ret < 0) {
            perror("Error writing segment");
            exit(1);
        }

        off_t seg_size = content_sz + calc_alignment(content_sz, PAGE_SIZE);
        off += seg_size;
        vaddr += seg_size;
        j += 1;
        free(content);
    }

    return j;
}


void incr_segments_off(Elf64_Phdr* phdr, Elf64_Ehdr* ehdr) {
    ehdr->e_shoff += PAGE_SIZE;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_PHDR && phdr[i].p_offset != 0x0)
            phdr[i].p_offset += PAGE_SIZE;

        if (phdr[i].p_type == PT_LOAD && phdr[i].p_offset == 0x0) {
            phdr[i].p_filesz += PAGE_SIZE;
            phdr[i].p_memsz += PAGE_SIZE;
        }
    }
}


void rearrange_vaddr(Elf64_Phdr* phdr, Elf64_Ehdr* ehdr) {
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_vaddr != 0x0 &&
            (phdr[i].p_type == PT_PHDR || phdr[i].p_offset == 0x0)) {
            phdr[i].p_vaddr -= PAGE_SIZE;
            phdr[i].p_paddr -= PAGE_SIZE;
        }
    }
}
