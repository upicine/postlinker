#include <elf.h>

#include "relocator.h"
#include "string.h"

void relocate(int fd, Elf_Data* rel_elf) {
    char* rel_shstr = read_shstr_tab(rel_elf->fd, rel_elf->shdr,
                                     rel_elf->ehdr);
    Elf64_Shdr* rel_shdr = rel_elf->shdr;
    for (int i = 0; i < rel_elf->ehdr->e_shnum; i++) {
        if (rel_shdr[i].sh_type == SHT_RELA) {

        }
    }
}