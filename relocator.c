#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relocator.h"


static Elf64_Shdr* search_shdr(const char* shdr_name, Elf_Data* elf,
                               char* shstr_tab) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        char* s_name = shstr_tab + shdr[i].sh_name;
        if (strcmp(s_name, shdr_name) == 0)
            return shdr + i;
    }
}


static size_t get_sym_tab(Elf64_Sym** sym_tab, Elf_Data *elf) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            *sym_tab = read_sym_tab(elf->fd, shdr + i);
            return shdr[i].sh_size;
        }
    }
}


static char* get_str_tab(Elf_Data *elf, char* shstr_tab) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_STRTAB
            && strcmp(STR_TAB_NAME, shstr_tab + shdr[i].sh_name) == 0)
            return read_str_tab(elf->fd, shdr + i);
    }
}


static Elf64_Sym* search_symbol(const char* sym_name, Elf64_Sym* sym_tab,
                                char* str_tab, size_t sym_tab_sz) {
    char* act_sym_name;
    for (int i = 0; i < sym_tab_sz; i++) {
        act_sym_name = str_tab + sym_tab[i].st_name;
        if (strcmp(act_sym_name, sym_name) == 0)
            return sym_tab + i;
    }
}


void relocate(int fd, Elf_Data* rel_elf, Elf_Data* exec_elf) {
    Elf64_Sym *exec_sym_tab, *rel_sym_tab;

    size_t rel_sym_sz = get_sym_tab(&rel_sym_tab, rel_elf);
    size_t exec_sym_sz = get_sym_tab(&exec_sym_tab, exec_elf);

    char* rel_shstr = read_shstr_tab(rel_elf->fd, rel_elf->shdr,
                                     rel_elf->ehdr);
    Elf64_Shdr* rel_shdr = rel_elf->shdr;

    for (int i = 0; i < rel_elf->ehdr->e_shnum; i++) {
        if (rel_shdr[i].sh_type == SHT_RELA) {
            char* s_name = rel_shstr + rel_shdr[i].sh_name + RELA_STR_LEN;
            Elf64_Rela* rel_rela = read_rela_tab(rel_elf->fd, &rel_shdr[i]);
            Elf64_Shdr* reloc_shdr = search_shdr(s_name, rel_elf, rel_shstr);
            free(rel_rela);
        }
    }

    free(rel_shstr);
}



