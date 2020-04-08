#ifndef POSTLINKER_POSTLINKER_H
#define POSTLINKER_POSTLINKER_H

#include "parser.h"

void incr_segments_off(Elf_Data* elf);

void rearrange_vaddr(Elf_Data* elf);

void add_new_segments(int out_fd, Elf_Data* rel_elf, Elf_Data** exec_elf);

void incr_sections_off(Elf_Data* elf);

#endif //POSTLINKER_POSTLINEKR_H
