#ifndef POSTLINKER_POSTLINKER_H
#define POSTLINKER_POSTLINKER_H

void incr_segments_off(Elf64_Phdr* phdr, Elf64_Ehdr* ehdr);

void rearrange_vaddr(Elf64_Phdr* phdr, Elf64_Ehdr* ehdr);

off_t calc_alignment(off_t off, uint64_t alignment);

off_t next_available_offset(Elf64_Ehdr *ehdr, Elf64_Phdr* phdr);

off_t next_available_vaddr(Elf64_Ehdr *ehdr, Elf64_Phdr* phdr);

#endif //POSTLINKER_POSTLINEKR_H
