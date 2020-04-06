#include <elf.h>
#include <stdlib.h>
#include <zconf.h>
#include <stdio.h>
#include <fcntl.h>

#include "utils.h"


int open_file(const char* filename) {
    int f = open(filename, O_RDONLY);
    if (!f) {
        perror("Error opening file");
        exit(1);
    }
    return f;
}


int create_file(const char* filename) {
    int f = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (!f) {
        perror("Error creating file");
        exit(1);
    }
    return f;
}


void write_eheader(int fd, Elf64_Ehdr* ehdr) {
    ssize_t ret = pwrite(fd, ehdr, sizeof(*ehdr), 0);
    if (ret < 0) {
        perror("Error writing elf header");
        exit(1);
    }
}


void write_pheader(int fd, Elf64_Phdr* phdr, Elf64_Ehdr* ehdr) {
    ssize_t ret = pwrite(fd, phdr, ehdr->e_phnum * ehdr->e_phentsize, HEADER_SIZE);
    if (ret < 0) {
        perror("Error writing program header");
        exit(1);
    }
}

char* read_bytes(int fd, off_t start, size_t size) {
    char* buffer = malloc(size);

    ssize_t rret = pread(fd, buffer, size, start);
    if (rret < 0) {
        perror("Error reading bytes from file");
        exit(1);
    }

    return buffer;
}

void copy_file(int fd_from, int fd_to, off_t start) {
    off_t off = start;
    ssize_t rret = 0, wret = 0;
    char* buffer[PAGE_SIZE];

    do {
        rret = pread(fd_from, buffer, PAGE_SIZE, off);
        if (rret < 0) {
            perror("Error copy file");
            exit(1);
        }

        if (rret != 0) {
            wret = pwrite(fd_to, buffer, PAGE_SIZE, off + PAGE_SIZE);
            if (wret < 0) {
                perror("Error copy file");
                exit(1);
            }
        }

        off += PAGE_SIZE;
    } while (rret != 0);
}