#ifndef POSTLINKER_ELFUTILS_H
#define POSTLINKER_ELFUTILS_H

#include "parser.h"

#define HEADER_SIZE 0x40
#define PAGE_SIZE 0x1000
#define BASE_ADDR 0x400000


int create_file(const char* filename);

int open_file(const char* filename);

void copy_file(int fd_from, int fd_to, off_t start);

void write_pheader(int fd, const Elf_Data* elf);

void write_eheader(int fd, const Elf_Data* elf);

void write_sheader(int fd, const Elf_Data* elf);

void write_addr(int fd, const void* addr, size_t addr_sz, off_t off);

#endif //POSTLINKER_ELFUTILS_H
