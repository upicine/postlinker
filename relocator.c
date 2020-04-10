#include <elf.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "relocator.h"
#include "utils.h"


static Elf64_Shdr* search_shdr(const char* shdr_name, Elf_Data* elf,
                               char* shstr_tab) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        char* s_name = shstr_tab + shdr[i].sh_name;
        if (strcmp(s_name, shdr_name) == 0)
            return shdr + i;
    }
}


static size_t get_sym_tab(int fd, Elf64_Sym** sym_tab, Elf_Data *elf) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            *sym_tab = read_sym_tab(fd, shdr + i);
            return shdr[i].sh_size;
        }
    }
}


static char* get_str_tab(int fd, Elf_Data *elf, char* shstr_tab) {
    Elf64_Shdr* shdr = elf->shdr;

    for (int i = 0; i < elf->ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_STRTAB
            && strcmp(STR_TAB_NAME, shstr_tab + shdr[i].sh_name) == 0) {
            return read_str_tab(fd, shdr + i);
        }
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


static void relocate_addr(int fd, Elf64_Addr sym_addr, Elf64_Rela* rela,
                          Elf64_Shdr* reloc_shdr) {
    Elf64_Addr instr_addr = reloc_shdr->sh_addr + rela->r_offset;
    Elf64_Off instr_off = reloc_shdr->sh_offset + rela->r_offset + PAGE_SIZE;
    Elf64_Xword r_type = ELF64_R_TYPE(rela->r_info);

    if (r_type == R_X86_64_PC32 || r_type == R_X86_64_PLT32) {
        Elf64_Word new_addr = (Elf64_Word)(sym_addr + rela->r_addend
                                           - instr_addr);
        write_addr(fd, &new_addr, sizeof(Elf64_Word), instr_off);
    } else if (r_type == R_X86_64_32 || r_type == R_X86_64_32S) {
        Elf64_Word  new_addr = (Elf64_Word)(sym_addr + rela->r_addend);
        write_addr(fd, &new_addr, sizeof(Elf64_Word), instr_off);
    } else if (r_type == R_X86_64_64) {
        Elf64_Xword new_addr = sym_addr + rela->r_addend;
        write_addr(fd, &new_addr, sizeof(Elf64_Xword), instr_off);
    }
}


void relocate(int fd, Elf_Data* rel_elf, Elf_Data* exec_elf) {
    Elf64_Sym *exec_sym_tab, *rel_sym_tab;
    size_t rel_sym_sz = get_sym_tab(rel_elf->fd, &rel_sym_tab, rel_elf);
    size_t exec_sym_sz = get_sym_tab(fd, &exec_sym_tab, exec_elf);
    char* rel_shstr = read_shstr_tab(rel_elf->fd, rel_elf->shdr,
                                     rel_elf->ehdr);
    char* exec_shstr = read_shstr_tab(fd, exec_elf->shdr,
                                      exec_elf->ehdr);
    char* rel_str = get_str_tab(rel_elf->fd, rel_elf, rel_shstr);
    char* exec_str = get_str_tab(fd ,exec_elf, exec_shstr);
    Elf64_Shdr* rel_shdr = rel_elf->shdr;

    for (int i = 0; i < rel_elf->ehdr->e_shnum; i++) {
        if (rel_shdr[i].sh_type == SHT_RELA) {
            char* s_name = rel_shstr + rel_shdr[i].sh_name + RELA_STR_LEN;
            Elf64_Rela* rel_rela = read_rela_tab(rel_elf->fd, &rel_shdr[i]);
            size_t rel_rela_sz = rel_shdr[i].sh_size / sizeof(Elf64_Rela);
            Elf64_Shdr* reloc_shdr = search_shdr(s_name, rel_elf, rel_shstr);

            for (int j = 0; j < rel_rela_sz; j++) {
                Elf64_Xword symndx = ELF64_R_SYM(rel_rela[j].r_info);
                Elf64_Shdr* sym_shdr;
                Elf64_Sym* sym;
                Elf64_Addr sym_addr;

                if (rel_sym_tab[symndx].st_shndx == SHN_UNDEF) {
                    char* sym_name = rel_str + rel_sym_tab[symndx].st_name;

                    if (strcmp(ORIG_START_NAME, sym_name) == 0) {
                        sym_addr = exec_elf->ehdr->e_entry;
                    } else {
                        sym = search_symbol(sym_name, exec_sym_tab, exec_str,
                                            exec_sym_sz);
//                        sym_shdr = exec_elf->shdr + sym->st_shndx;
                        sym_addr = sym->st_value;
                    }
                } else {
                    char* sym_name = rel_str + rel_sym_tab[symndx].st_name;
                    sym_shdr = rel_elf->shdr + rel_sym_tab[symndx].st_shndx;
                    sym_addr = sym_shdr->sh_addr + rel_sym_tab[symndx].st_value;
                }

                relocate_addr(fd, sym_addr, rel_rela + j, reloc_shdr);
            }

            free(rel_rela);
        }
    }

    Elf64_Sym* start_sym = search_symbol(_START_NAME, rel_sym_tab, rel_str,
                                         rel_sym_sz);
    Elf64_Shdr* start_hdr = rel_shdr + start_sym->st_shndx;
    exec_elf->ehdr->e_entry = start_hdr->sh_addr + start_sym->st_value;

    free(exec_sym_tab);
    free(rel_sym_tab);
    free(rel_shstr);
    free(exec_shstr);
    free(rel_str);
    free(exec_str);
}



