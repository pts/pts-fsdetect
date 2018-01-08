#include "fsdetect_impl.h"

#if defined(__TINYC__)
#pragma pack(push, 1)
#endif

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
  struct btrfs_dev_item {  /* A complete block device. */
    uint64_t devid;
    uint64_t total_bytes;
    uint64_t bytes_used;
    uint32_t io_align;
    uint32_t io_width;
    uint32_t sector_size;
    /* Bit field:
     * BTRFS_BLOCK_GROUP_DATA [0x1]
     * BTRFS_BLOCK_GROUP_SYSTEM [0x2]
     * BTRFS_BLOCK_GROUP_METADATA [0x4]
     */
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

#if defined(__TINYC__)
#pragma pack(pop)
#endif

struct Assert512BytesStruct {
   int Assert512Bytes : sizeof(struct btrfs_super_block) == 512; };

/* Code based on util-linux-2.31/libblkid/src/superblocks/btrfs.c */
int fsdetect_btrfs(read_block_t read_block, void *read_block_data,
                   struct fsdetect_output *fsdo) {
  struct btrfs_super_block sb;
  if (read_block(read_block_data, 128, 1, &sb) != 0) return 10;
  /* https://btrfs.wiki.kernel.org/index.php/On-disk_Format#Superblock */
  /* https://btrfs.wiki.kernel.org/index.php/Data_Structures#btrfs_super_block */
  if (0 != memcmp(sb.magic, "_BHRfS_M", 8)) return 11;
  if (le16(sb.csum_type) == 0 && !(sb.csum[4] == 0 && sb.csum[5] == 0 && sb.csum[6] == 0 && sb.csum[7] == 0)) return 12;
  /* if (le64(sb.flags) == 0) return 13; */  /* BTRFS_HEADER_FLAG_WRITTEN? */
  if (le64(sb.generation) < 4) return 14;
  if (le64(sb.root) == le64(sb.chunk_root) || le64(sb.root) == le64(sb.log_root) || le64(sb.chunk_root) == le64(sb.log_root)) return 15;
  if (le64(sb.total_bytes) < le64(sb.bytes_used)) return 16;
  if (le64(sb.root_dir_objectid) < 6) return 17;
  if (le64(sb.num_devices) == 0) return 18;
  if (le64(sb.num_devices) > 0x200) return 19;
  if (sb.sectorsize < 0x1000) return 20;
  if (!is_power_of_2(sb.sectorsize)) return 21;
  if (!is_power_of_2(le64(sb.nodesize)) || le64(sb.nodesize) > 0x10000) return 22;
  if (le64(sb.nodesize) < sb.sectorsize) return 23;
  if (!is_power_of_2(le64(sb.leafsize)) || le64(sb.leafsize) > 0x10000) return 24;
  if (le64(sb.leafsize) < sb.sectorsize) return 25;
  if (!is_power_of_2(sb.stripesize)) return 26;
  if (sb.stripesize < sb.sectorsize) return 27;
  if (le64(sb.chunk_root_generation) < 4) return 28;
  if (le64(sb.dev_item.devid) == 0) return 29;
  if (le64(sb.dev_item.total_bytes) > le64(sb.total_bytes)) return 30;
  if (le64(sb.dev_item.total_bytes) < le64(sb.dev_item.bytes_used)) return 31;
  if (sb.dev_item.io_align < 0x1000) return 32;
  if (!is_power_of_2(sb.dev_item.io_align >> 12)) return 33;
  if (sb.dev_item.io_width < 0x1000) return 34;
  if (!is_power_of_2(sb.dev_item.io_width >> 12)) return 35;
  if (le64(sb.dev_item.sector_size) < 0x1000) return 36;
  if (!is_power_of_2(sb.dev_item.sector_size >> 12)) return 37;

#ifdef DEBUG
  /* Typical example:
   * $ dd if=/dev/zero of=fs.btrfs bs=1M count=100
   * $ /sbin/mkfs.btrfs fs.btrfs
   * csum=<f1a88fc600000000>
   * bytenr=0x10000
   * flags=0x1
   * generation=0x4
   * root=0x405000
   * chunk_root=0x20000
   * log_root=0x0
   * log_root_transid=0x0
   * total_bytes=0x6400000
   * bytes_used=0x7000
   * root_dir_objectid=0x6
   * num_devices=0x1
   * sectorsize=0x1000
   * nodesize=0x1000
   * leafsize=0x1000
   * stripesize=0x1000
   * sys_chunk_array_size=0x61
   * chunk_root_generation=0x4
   * compat_flags=0x0
   * compat_ro_flags=0x0
   * incompat_flags=0x45
   * csum_type=0x0
   * root_level=0x0
   * chunk_root_level=0x0
   * log_root_level=0x0
   * dev_item.devid=0x1
   * dev_item.total_bytes=0x6400000
   * dev_item.bytes_used=0xc00000
   * dev_item.io_align=0x1000
   * dev_item.io_width=0x1000
   * dev_item.sector_size=0x1000
   * dev_item.type=0x0
   * dev_item.generation=0x0
   * dev_item.start_offset=0x0
   * dev_item.dev_group=0x0
   * dev_item.seek_speed=0x0
   * dev_item.bandwidth=0x0
   */
  __extension__ printf("csum=<%02x%02x%02x%02x%02x%02x%02x%02x>\n", sb.csum[0], sb.csum[1], sb.csum[2], sb.csum[3], sb.csum[4], sb.csum[5], sb.csum[6], sb.csum[7]);
  __extension__ printf("bytenr=0x%llx\n", (long long)le64(sb.bytenr));  /* Physical address of this block. */
  __extension__ printf("flags=0x%llx\n", (long long)le64(sb.flags));
  __extension__ printf("generation=0x%llx\n", (long long)le64(sb.generation));
  __extension__ printf("root=0x%llx\n", (long long)le64(sb.root));
  __extension__ printf("chunk_root=0x%llx\n", (long long)le64(sb.chunk_root));
  __extension__ printf("log_root=0x%llx\n", (long long)le64(sb.log_root));
  __extension__ printf("log_root_transid=0x%llx\n", (long long)le64(sb.log_root_transid));
  __extension__ printf("total_bytes=0x%llx\n", (long long)le64(sb.total_bytes));
  __extension__ printf("bytes_used=0x%llx\n", (long long)le64(sb.bytes_used));
  __extension__ printf("root_dir_objectid=0x%llx\n", (long long)le64(sb.root_dir_objectid));
  __extension__ printf("num_devices=0x%llx\n", (long long)le64(sb.num_devices));
  __extension__ printf("sectorsize=0x%x\n", (int)le32(sb.sectorsize));
  __extension__ printf("nodesize=0x%x\n", (int)le32(sb.nodesize));
  __extension__ printf("leafsize=0x%x\n", (int)le32(sb.leafsize));
  __extension__ printf("stripesize=0x%x\n", (int)le32(sb.stripesize));
  __extension__ printf("sys_chunk_array_size=0x%x\n", (int)le32(sb.sys_chunk_array_size));
  __extension__ printf("chunk_root_generation=0x%llx\n", (long long)le64(sb.chunk_root_generation));
  __extension__ printf("compat_flags=0x%llx\n", (long long)le64(sb.compat_flags));
  __extension__ printf("compat_ro_flags=0x%llx\n", (long long)le64(sb.compat_ro_flags));
  __extension__ printf("incompat_flags=0x%llx\n", (long long)le64(sb.incompat_flags));
  __extension__ printf("csum_type=0x%x\n", (int)le16(sb.csum_type));
  __extension__ printf("root_level=0x%x\n", sb.root_level);
  __extension__ printf("chunk_root_level=0x%x\n", sb.chunk_root_level);
  __extension__ printf("log_root_level=0x%x\n", sb.log_root_level);
  __extension__ printf("dev_item.devid=0x%llx\n", (long long)le64(sb.dev_item.devid));
  __extension__ printf("dev_item.total_bytes=0x%llx\n", (long long)le64(sb.dev_item.total_bytes));
  __extension__ printf("dev_item.bytes_used=0x%llx\n", (long long)le64(sb.dev_item.bytes_used));
  __extension__ printf("dev_item.io_align=0x%x\n", (int)le32(sb.dev_item.io_align));
  __extension__ printf("dev_item.io_width=0x%x\n", (int)le32(sb.dev_item.io_width));
  __extension__ printf("dev_item.sector_size=0x%x\n", (int)le32(sb.dev_item.sector_size));
  __extension__ printf("dev_item.type=0x%llx\n", (long long)le64(sb.dev_item.type));
  __extension__ printf("dev_item.generation=0x%llx\n", (long long)le64(sb.dev_item.generation));
  __extension__ printf("dev_item.start_offset=0x%llx\n", (long long)le64(sb.dev_item.start_offset));
  __extension__ printf("dev_item.dev_group=0x%x\n", (int)le32(sb.dev_item.dev_group));
  __extension__ printf("dev_item.seek_speed=0x%x\n", sb.dev_item.seek_speed);
  __extension__ printf("dev_item.bandwidth=0x%x\n", sb.dev_item.bandwidth);
#endif

  strcpy(fsdo->fstype, "btrfs");
  strncpy(fsdo->label, (const char*)sb.label, 16);
  fsdo->label[16] = '\0';
  fsdo->uuid_size = 16;
  memcpy(fsdo->uuid, sb.fsid, 16);
  return 0;
}
