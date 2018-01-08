#include "fsdetect_impl.h"

void fsdetect(read_block_t read_block, void *read_block_data,
              struct fsdetect_output *fsdo) {
  memset(fsdo, '\0', sizeof(*fsdo));
  /* Syslinux 4.07 ldlinux.lst has the filesystems in this order. */
  if (fsdetect_fat(read_block, read_block_data, fsdo) != 0 &&
      fsdetect_ext(read_block, read_block_data, fsdo) != 0 &&
      fsdetect_ntfs(read_block, read_block_data, fsdo) != 0 &&
      fsdetect_btrfs(read_block, read_block_data, fsdo) != 0) {
    memset(fsdo, '\0', sizeof(*fsdo));
    fsdo->fstype[0] = '?';
  }
}
