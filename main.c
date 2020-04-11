#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "postlinker.h"
#include "relocator.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage ./%s <ET_EXEC file> <ET_REL file> <ET_EXEC output file>",
               argv[0]);
        exit(1);
    }

    int output_fd = create_file(argv[3]);

    Elf_Data* exec_elf =  init_exec_elf(argv[1]);
    Elf_Data* rel_elf = init_rel_elf(argv[2]);

    set_base_vaddr(exec_elf);

    incr_segments_off(exec_elf);
    rearrange_vaddr(exec_elf);
    copy_file(exec_elf->fd, output_fd, 0x0);

    add_new_segments(output_fd, rel_elf, &exec_elf);
    incr_sections_off(exec_elf);

    relocate(output_fd, rel_elf, exec_elf);

    write_eheader(output_fd, exec_elf);
    write_pheader(output_fd, exec_elf);
    write_sheader(output_fd, exec_elf);

    free_elf(&exec_elf);
    free_elf(&rel_elf);
    int ret = close(output_fd);
    if (ret < 0) {
        perror("Error closing output file");
        exit(1);
    }

    return 0;
}