//
// Created by michal on 05.04.20.
//

#include <elf.h>
#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>

#include "parser.h"



Elf64_Ehdr* read_elf_header(int fd) {
    Elf64_Ehdr* ehdr = malloc(sizeof(Elf64_Ehdr));

    ssize_t ret = read(fd, ehdr, sizeof(*ehdr));
    if (ret < 0) {
        perror("Error reading file");
        exit(1);
    }

    return ehdr;
}


Elf64_Shdr* read_sections_header(int fd, Elf64_Ehdr *ehdr) {
    size_t sect_size = ehdr->e_shentsize * ehdr->e_shnum;
    Elf64_Shdr* sections = malloc(sect_size);

    ssize_t ret = pread(fd, sections, sect_size, ehdr->e_shoff);
    if (ret < 0) {
        perror("Error reading file");
        exit(1);
    }

    return sections;
}


Elf64_Phdr* read_program_header(int fd, Elf64_Ehdr* ehdr) {
    size_t phdr_size = ehdr->e_phentsize * ehdr->e_phnum;
    Elf64_Phdr* phdr = malloc(phdr_size);

    ssize_t ret = pread(fd, phdr, phdr_size, ehdr->e_phoff);
    if (ret < 0) {
        perror("Error reading file");
        exit(1);
    }

    return  phdr;
}