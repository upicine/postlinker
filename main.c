#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <unistd.h>
#include <assert.h>

#include "utils.h"
#include "postlinker.h"
#include "parser.h"


int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage ./postlinker <ET_EXEC file> <ET_REL file> <ET_EXEC output file>");
        exit(1);
    }

    int exec_fd = open_file(argv[1]);
    int rel_fd = open_file(argv[2]);
    int output_fd = create_file(argv[3]);

    Elf64_Ehdr* rel_ehdr = read_elf_header(rel_fd);
    Elf64_Ehdr* exec_ehdr = read_elf_header(exec_fd);

    Elf64_Phdr* exec_phdr = read_program_header(exec_fd, exec_ehdr);

    Elf64_Shdr* rel_shdr = read_sections_header(rel_fd, rel_ehdr);
    Elf64_Shdr* exec_shdr = read_sections_header(exec_fd, exec_ehdr);

    printf("%x\n", next_available_offset(exec_ehdr, exec_phdr));
    printf("%x\n", next_available_vaddr(exec_ehdr, exec_phdr));
//
//    incr_segments_off(exec_phdr, exec_ehdr);
//    rearrange_vaddr(exec_phdr, exec_ehdr);
//    write_eheader(output_fd, exec_ehdr);
//    write_pheader(output_fd, exec_phdr, exec_ehdr);
//    copy_file(exec_fd, output_fd, 0x2a8);
//
    free(rel_ehdr);
    free(exec_ehdr);
    free(exec_phdr);

    close(exec_fd);
    close(rel_fd);
    close(output_fd);

    return 0;
}