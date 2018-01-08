#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "fsdetect.h"

static int fd_read_block(void *fd_ptr, uint32_t block_idx,
                         uint32_t block_count, void *buf) {
  const off_t ofs = (off_t)block_idx << 9;
  const size_t size = (size_t)block_count << 9;
  const int fd = (size_t)fd_ptr;
  if (ofs != lseek(fd, ofs, SEEK_SET)) { err:
    memset(buf, '\0', size);
    return -1;
  }
  if (size != (size_t)read(fd, buf, size)) goto err;
  return 0;
}

int main(int argc, char **argv) {
  struct fsdetect_output fsdo;

  (void)argc; (void)argv;
  fsdetect(fd_read_block, (void*)0, &fsdo);  /* stdin. */
  printf("fstype=%s\n", fsdo.fstype);  /* Can be "?". */
  printf("label=%s\n", fsdo.label);  /* Can be empty. */
  if (fsdo.uuid_size == 0) {
    printf("uuid=\n");
  } else if (fsdo.uuid_size == 4) {
    const unsigned char *up = fsdo.uuid;
    printf("uuid=%02X%02X-%02X%02X\n",
           up[0], up[1], up[2], up[3]);
  } else if (fsdo.uuid_size == 8) {  /* NTFS. */
    const unsigned char *up = fsdo.uuid;
    printf("uuid=%02X%02X%02X%02X%02X%02X%02X%02X\n",
           up[0], up[1], up[2], up[3], up[4], up[5], up[6], up[7]);
  } else if (fsdo.uuid_size == 16) {
    const unsigned char *up = fsdo.uuid;
    printf("uuid=%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
           "%02x%02x%02x%02x%02x%02x\n",
           up[0], up[1], up[2], up[3], up[4], up[5], up[6], up[7],
           up[8], up[9], up[10], up[11], up[12], up[13], up[14], up[15]);
  } else {
    printf("uuid=?%d\n", fsdo.uuid_size);
  }
  return 0;
}
