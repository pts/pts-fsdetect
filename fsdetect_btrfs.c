#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "fsdetect_impl.h"

struct btrfs_super_block {
  uint8_t csum[32];
  uint8_t fsid[16];
  uint64_t bytenr;
  uint64_t flags;
  uint8_t magic[8];
  uint64_t generation;
  uint64_t root;
  uint64_t chunk_root;
  uint64_t log_root;
  uint64_t log_root_transid;
  uint64_t total_bytes;
  uint64_t bytes_used;
  uint64_t root_dir_objectid;
  uint64_t num_devices;
  uint32_t sectorsize;
  uint32_t nodesize;
  uint32_t leafsize;
  uint32_t stripesize;
  uint32_t sys_chunk_array_size;
  uint64_t chunk_root_generation;
  uint64_t compat_flags;
  uint64_t compat_ro_flags;
  uint64_t incompat_flags;
  uint16_t csum_type;
  uint8_t root_level;
  uint8_t chunk_root_level;
  uint8_t log_root_level;
  struct btrfs_dev_item {
    uint64_t devid;
    uint64_t total_bytes;
    uint64_t bytes_used;
    uint32_t io_align;
    uint32_t io_width;
    uint32_t sector_size;
    uint64_t type;
    uint64_t generation;
    uint64_t start_offset;
    uint32_t dev_group;
    uint8_t seek_speed;
    uint8_t bandwidth;
    uint8_t uuid[16];
    uint8_t fsid[16];
  } __attribute__ ((__packed__)) dev_item;
  uint8_t label[213];
} __attribute__ ((__packed__));

struct Assert512BytesStruct {
     int Assert512Bytes : sizeof(struct btrfs_super_block) == 512; };

/* Code based on util-linux-2.31/libblkid/src/superblocks/btrfs.c */
int fsdetect_btrfs(read_block_t read_block, void *read_block_data,
                   struct fsdetect_output *fsdo) {
  struct btrfs_super_block sb;
  if (read_block(read_block_data, 128, 1, &sb) != 0) return 10;
  if (0 != memcmp(sb.magic, "_BHRfS_M", 8)) return 11;
  strcpy(fsdo->fstype, "btrfs");
  strncpy(fsdo->label, (const char*)sb.label, 16);
  fsdo->label[16] = '\0';
  fsdo->uuid_size = 16;
  memcpy(fsdo->uuid, sb.fsid, 16);
  return 0;
}
