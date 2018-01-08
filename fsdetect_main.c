/* No need for this, we don't seek too far: #define _FILE_OFFSET_BITS 64 */
#ifdef __XTINY__
#include <xtiny.h>
#else
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "fsdetect.h"

#if defined(__i386) || defined(__amd64)
#define REGPARM3 __attribute__((regparm(3)))
#else
#define REGPARM3
#endif

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

REGPARM3 static __inline__ char *emit_char(char *p, char c) {
  *p++ = c;
  return p;
}

REGPARM3 static char *emit_asciiz(char *p, const char *asciiz) {
  const size_t size = strlen(asciiz);
  memcpy(p, asciiz, size);
  return p + size;
}

REGPARM3 static char *emit_hex(char *p, const char *bin, size_t size, char is_uc) {
  for (; size > 0; --size) {
    const unsigned char c = *bin++;
    unsigned char n = c >> 4;
    *p++ = n <= 9 ? n + '0' : n + (is_uc ? 'A' - 10 : 'a' - 10);
    n = c & 15;
    *p++ = n <= 9 ? n + '0' : n + (is_uc ? 'A' - 10 : 'a' - 10);
  }
  return p;
}

int main(int argc, char **argv) {
  struct fsdetect_output fsdo;
  char outbuf[256], *p = outbuf;

  (void)argc; (void)argv;
  fsdetect(fd_read_block, (void*)0  /* stdin */, &fsdo);
  /* fsdo.fstype can be "?", fsdo.label can be empty, fsdo.uuid_size can be 0. */
  p = emit_asciiz(emit_asciiz(emit_asciiz(emit_asciiz(emit_asciiz(p, "fstype="), fsdo.fstype), "\nlabel="), fsdo.label), "\nuuid=");
  if (fsdo.uuid_size == 0) {
    p = emit_char(p, '?');
  } else if (fsdo.uuid_size == 4) {  /* FAT. */
    p = emit_hex(emit_char(emit_hex(p, (const char*)fsdo.uuid, 2, 1), '-'), (const char*)fsdo.uuid + 2, 2, 1);
  } else if (fsdo.uuid_size == 8) {  /* NTFS. */
    p = emit_hex(p, (const char*)fsdo.uuid, 8, 1);
  } else if (fsdo.uuid_size == 16) {  /* ext2 and Btrfs. */
    p = emit_hex(emit_char(emit_hex(emit_char(emit_hex(emit_char(emit_hex(emit_char(emit_hex(p, (const char*)fsdo.uuid, 4, 0), '-'), (const char*)fsdo.uuid + 4, 2, 0), '-'), (const char*)fsdo.uuid + 6, 2, 0), '-'), (const char*)fsdo.uuid + 8, 2, 0), '-'), (const char*)fsdo.uuid + 10, 6, 0);
  } else {
    p = emit_asciiz(p, "?s");
  }
  p = emit_char(p, '\n');
  (void)!write(1, outbuf, p - outbuf);
  return 0;
}
