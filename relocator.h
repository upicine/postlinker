#ifndef POSTLINKER_RELOCATION_H
#define POSTLINKER_RELOCATION_H

#include "parser.h"

#define RELA_STR_LEN 5
#define STR_TAB_NAME ".strtab"
#define ORIG_START_NAME "orig_start"
#define _START_NAME "_start"

void relocate(int fd, Elf_Data* rel_elf, Elf_Data* exec_elf);

#endif //POSTLINKER_RELOCATION_H
