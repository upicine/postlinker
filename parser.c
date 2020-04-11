#include <elf.h>
#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>

#include "parser.h"
#include "utils.h"


Elf_Data* init_exec_elf(const char* filename) {
    Elf_Data* exec_elf = malloc(sizeof(Elf_Data));
    if (!exec_elf) {
        perror("Error init exec elf");
        exit(1);
    }

    exec_elf->fd = open_file(filename);
    exec_elf->ehdr = read_elf_header(exec_elf->fd);
    exec_elf->phdr = read_program_header(exec_elf->fd, exec_elf->ehdr);
    exec_elf->shdr = read_sections_header(exec_elf->fd, exec_elf->ehdr);
}


Elf_Data* init_rel_elf(const char* filename) {
    Elf_Data* rel_elf = malloc(sizeof(Elf_Data));
    if (!rel_elf) {
        perror("Error init rel elf");
        exit(1);
    }

    rel_elf->fd = open_file(filename);
    rel_elf->ehdr = read_elf_header(rel_elf->fd);
    rel_elf->phdr = NULL;
    rel_elf->shdr = read_sections_header(rel_elf->fd, rel_elf->ehdr);
}


void free_elf(Elf_Data** elf) {
    int ret = close((*elf)->fd);
    if (ret < 0) {
        perror("Error closing elf file");
        exit(1);

    }
    free((*elf)->ehdr);
    free((*elf)->phdr);
    free((*elf)->shdr);
    free(*elf);
}


Elf64_Ehdr* read_elf_header(int fd) {
    Elf64_Ehdr* ehdr = malloc(sizeof(Elf64_Ehdr));
    if (!ehdr) {
        perror("Error reading elf header");
        exit(1);
    }

    ssize_t ret = read(fd, ehdr, sizeof(*ehdr));
    if (ret < 0) {
        perror("Error reading elf header");
        exit(1);
    }

    return ehdr;
}


Elf64_Shdr* read_sections_header(int fd, const Elf64_Ehdr *ehdr) {
    size_t sect_size = ehdr->e_shentsize * ehdr->e_shnum;
    Elf64_Shdr* sections = malloc(sect_size);
    if (!sections && sect_size > 0) {
        perror("Error reading sections header");
        exit(1);
    }

    ssize_t ret = pread(fd, sections, sect_size, ehdr->e_shoff);
    if (ret < 0) {
        perror("Error reading sections header");
        exit(1);
    }

    return sections;
}


Elf64_Phdr* read_program_header(int fd, const Elf64_Ehdr* ehdr) {
    size_t phdr_size = ehdr->e_phentsize * ehdr->e_phnum;
    Elf64_Phdr* phdr = malloc(phdr_size);
    if (!phdr && phdr_size > 0) {
        perror("Error reading program header");
        exit(1);
    }

    ssize_t ret = pread(fd, phdr, phdr_size, ehdr->e_phoff);
    if (ret < 0) {
        perror("Error reading program header");
        exit(1);
    }

    return  phdr;
}


char* read_shstr_tab(int fd, const Elf64_Shdr* shdr, const Elf64_Ehdr* ehdr) {
    Elf64_Shdr shstr_hdr = shdr[ehdr->e_shstrndx];
    char* shstr_tab = malloc(shstr_hdr.sh_size);
    if (!shstr_tab && shstr_hdr.sh_size > 0) {
        perror("Error reading shstr tab");
        exit(1);
    }

    ssize_t ret = pread(fd, shstr_tab, shstr_hdr.sh_size, shstr_hdr.sh_offset);
    if (ret < 0) {
        perror("Error reading shstr tab");
        exit(1);
    }

    return shstr_tab;
}


Elf64_Rela* read_rela_tab(int fd, const Elf64_Shdr* rela_sh) {
    Elf64_Rela* rela = malloc(rela_sh->sh_size);
    if (!rela && rela_sh->sh_size != 0) {
        perror("Error reading rela tab");
        exit(1);
    }

    ssize_t ret = pread(fd, rela, rela_sh->sh_size, rela_sh->sh_offset);
    if (ret < 0) {
        perror("Error reading rela tab");
        exit(1);
    }

    return rela;
}


Elf64_Sym* read_sym_tab(int fd, const Elf64_Shdr* sym_shdr) {
    Elf64_Sym* sym_tab = malloc(sym_shdr->sh_size);
    if (!sym_tab && sym_shdr->sh_size != 0) {
        perror("Error reading sym tab");
        exit(1);
    }

    ssize_t ret = pread(fd, sym_tab, sym_shdr->sh_size, sym_shdr->sh_offset);
    if (ret < 0) {
        perror("Error reading rela tab");
        exit(1);
    }

    return sym_tab;
}

char* read_str_tab(int fd, const Elf64_Shdr* shdr) {
    char* str_tab = malloc(shdr->sh_size);
    if (!str_tab && shdr->sh_size > 0) {
        perror("Error reading shstr tab");
        exit(1);
    }

    ssize_t ret = pread(fd, str_tab, shdr->sh_size, shdr->sh_offset);
    if (ret < 0) {
        perror("Error reading shstr tab");
        exit(1);
    }

    return str_tab;
}
