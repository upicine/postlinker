#ifndef POSTLINKER_PARSER_H_H
#define POSTLINKER_PARSER_H_H

Elf64_Ehdr* read_elf_header(int fd);

Elf64_Shdr* read_sections_header(int fd, Elf64_Ehdr *ehdr);

Elf64_Phdr* read_program_header(int fd, Elf64_Ehdr* ehdr);

#endif //POSTLINKER_PARSER_H_H
