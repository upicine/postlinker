#ifndef POSTLINKER_ELFUTILS_H
#define POSTLINKER_ELFUTILS_H

#define HEADER_SIZE 0x40
#define PAGE_SIZE 0x1000
#define BASE_ADDR 0x400000

int create_file(const char* filename);

int open_file(const char* filename);

void copy_file(int fd_from, int fd_to, off_t start);

void write_pheader(int fd, Elf64_Phdr* phdr, Elf64_Ehdr* ehdr);

void write_eheader(int fd, Elf64_Ehdr* ehdr);

char* read_bytes(int fd, off_t start, size_t size);


#endif //POSTLINKER_ELFUTILS_H
