#ifndef _FSDETECT_H
#define _FSDETECT_H 1

#include <stdint.h>

struct fsdetect_output {
  char fstype[14];  /* e.g. "ext2". */
  char label[17];  /* Last character is \0. */
  char uuid_size;  /* 0 = UUID not found. */
  uint8_t uuid[16];  /* Binary (not hex). */
};

/* Each block is 512 bytes. */
typedef int (*read_block_t)(
    void *fd_ptr, uint32_t block_idx, uint32_t block_count, void *buf);

void fsdetect(read_block_t read_block, void *read_block_data,
              struct fsdetect_output *fsdo);

#endif /* _FSDETECT_H */
