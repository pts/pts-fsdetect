#include <string.h>
#include "fsdetect_impl.h"

void fsdetect(read_block_t read_block, void *read_block_data,
              struct fsdetect_output *fsdo) {
  memset(fsdo, '\0', sizeof(*fsdo));
  if (fsdetect_ext(read_block, read_block_data, fsdo) != 0) {
    memset(fsdo, '\0', sizeof(*fsdo));
    fsdo->fstype[0] = '?';
  }
}
