#ifndef POSTLINKER_PARSER_H_H
#define POSTLINKER_PARSER_H_H


typedef struct Elf_File {
    int fd;
    Elf64_Ehdr* ehdr;
    Elf64_Phdr* phdr;
    Elf64_Shdr* shdr;
} Elf_Data;


Elf_Data* init_exec_elf(const char* filename);

Elf_Data* init_rel_elf(const char* filename);

void free_elf(Elf_Data** elf);

Elf64_Ehdr* read_elf_header(int fd);

Elf64_Shdr* read_sections_header(int fd, const Elf64_Ehdr* ehdr);

Elf64_Phdr* read_program_header(int fd, const Elf64_Ehdr* ehdr);

char* read_shstr_tab(int fd, const Elf64_Shdr* shdr, const Elf64_Ehdr* ehdr);

Elf64_Rela* read_rela_tab(int fd, const Elf64_Shdr* rela_sh);

#endif //POSTLINKER_PARSER_H_H
