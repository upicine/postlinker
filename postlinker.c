#include <elf.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "postlinker.h"
#include "utils.h"

static const Elf64_Xword sect_flags[] = {
        SHF_ALLOC + SHF_EXECINSTR + SHF_WRITE,
        SHF_ALLOC + SHF_EXECINSTR,
        SHF_ALLOC + SHF_WRITE,
        SHF_ALLOC
};

static const size_t sect_flags_sz = 4;


static Elf64_Word shflag_to_phflag(Elf64_Xword shflag) {
    switch (shflag) {
        case SHF_ALLOC + SHF_EXECINSTR + SHF_WRITE:
            return PF_R + PF_W + PF_X;
        case SHF_ALLOC + SHF_EXECINSTR:
            return PF_R + PF_X;
        case SHF_ALLOC + SHF_WRITE:
            return PF_R + PF_W;
        case SHF_ALLOC:
            return PF_R;
    }
}


static off_t calc_alignment(off_t off, uint64_t alignment) {
    if (off % alignment != 0)
        return alignment - (off % alignment);

    return 0;
}


static Elf64_Off next_avail_off(int fd) {
    off_t file_sz;

    file_sz = lseek(fd, 0, SEEK_END);
    if (file_sz < 0) {
        perror("Error reading last available offset");
        exit(1);
    }

    return (Elf64_Off) file_sz + calc_alignment(file_sz, PAGE_SIZE);
}


static size_t merge_content(char** content, Elf_Data* elf, Elf64_Xword flag,
                            Elf64_Addr vaddr) {
    Elf64_Ehdr* ehdr = elf->ehdr;
    Elf64_Shdr* shdr = elf->shdr;
    size_t content_sz = 0;
    off_t off = 0, addr_align = 0;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_size != 0 && shdr[i].sh_flags == flag) {
            addr_align = calc_alignment(off, shdr[i].sh_addralign);

            content_sz += addr_align + shdr[i].sh_size;
            *content = realloc(*content, content_sz);
            if (!*content) {
                perror("Error merge segments");
                exit(1);
            }

            ssize_t ret = pread(elf->fd, *content + off + addr_align, shdr[i].sh_size,
                                shdr[i].sh_offset);
            if (ret < 0) {
                perror("Error merge segments");
                exit(1);
            }

            shdr[i].sh_addr = vaddr + off + addr_align;
            off += addr_align + shdr[i].sh_size;
        }
    }

    return content_sz;
}


uint16_t merge_sections(int out_fd, Elf_Data* exec_elf, Elf_Data* rel_elf,
                        Elf64_Phdr* new_phdr) {
    char* content = NULL;
    uint16_t j = 0;
    Elf64_Off off = next_avail_off(exec_elf->fd);

    for (int i = 0; i < sect_flags_sz; i++) {
        Elf64_Addr vaddr = BASE_ADDR + off;
        size_t content_sz = merge_content(&content, rel_elf, sect_flags[i],
                                          vaddr);

        if (content_sz == 0)
            continue;

        new_phdr[j].p_type = PT_LOAD;
        new_phdr[j].p_flags = shflag_to_phflag(sect_flags[i]);
        new_phdr[j].p_offset = off + PAGE_SIZE;
        new_phdr[j].p_memsz = content_sz;
        new_phdr[j].p_filesz = content_sz;
        new_phdr[j].p_vaddr = vaddr;
        new_phdr[j].p_paddr = vaddr;
        new_phdr[j].p_align = PAGE_SIZE;

        ssize_t ret = pwrite(out_fd, content, content_sz, off + PAGE_SIZE);
        if (ret < 0) {
            perror("Error writing segment %");
            exit(1);
        }

        off += content_sz + calc_alignment(content_sz, PAGE_SIZE);
        j += 1;
        free(content);
        content = NULL;
    }

    return j;
}


void add_new_segments(int out_fd, Elf_Data* rel_elf, Elf_Data** exec_elf) {
    Elf64_Phdr new_phdr[sect_flags_sz];
    Elf64_Ehdr* exec_ehdr = (*exec_elf)->ehdr;
    uint16_t new_seg_num = merge_sections(out_fd, *exec_elf, rel_elf, new_phdr);
    uint32_t new_phdr_sz = ((*exec_elf)->ehdr->e_phnum + new_seg_num)
                           * (*exec_elf)->ehdr->e_phentsize;

    (*exec_elf)->phdr = realloc((*exec_elf)->phdr, new_phdr_sz);
    if (!(*exec_elf)->phdr && new_phdr_sz != 0) {
        perror("Error adding new segment");
        exit(1);
    }

    Elf64_Phdr* exec_phdr = (*exec_elf)->phdr;
    memcpy(exec_phdr + exec_ehdr->e_phnum, new_phdr, new_seg_num * sizeof(Elf64_Phdr));
    for (int i = 0; i < exec_ehdr->e_phnum; i++) {
        if (exec_phdr[i].p_type == PT_PHDR) {
            exec_phdr[i].p_filesz = new_phdr_sz;
            exec_phdr[i].p_memsz = new_phdr_sz;
        }
    }

    exec_ehdr->e_phnum += new_seg_num;
}


void incr_segments_off(Elf_Data* elf) {
    Elf64_Phdr* phdr = elf->phdr;

    elf->ehdr->e_shoff += PAGE_SIZE;

    for (int i = 0; i < elf->ehdr->e_phnum; i++) {
        if (phdr[i].p_type != PT_PHDR && phdr[i].p_offset != 0x0)
            phdr[i].p_offset += PAGE_SIZE;

        if (phdr[i].p_type == PT_LOAD && phdr[i].p_offset == 0x0) {
            phdr[i].p_filesz += PAGE_SIZE;
            phdr[i].p_memsz += PAGE_SIZE;
        }
    }
}


void incr_sections_off(Elf_Data* elf) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        if (shdr[i].sh_offset != 0x0) {
            shdr[i].sh_offset += PAGE_SIZE;
        }
    }
}


void rearrange_vaddr(Elf_Data* elf) {
    Elf64_Phdr* phdr = elf->phdr;

    for (int i = 0; i < elf->ehdr->e_phnum; i++) {
        if (phdr[i].p_vaddr != 0x0 &&
            (phdr[i].p_type == PT_PHDR || phdr[i].p_offset == 0x0)) {
            phdr[i].p_vaddr -= PAGE_SIZE;
            phdr[i].p_paddr -= PAGE_SIZE;
        }
    }
}
