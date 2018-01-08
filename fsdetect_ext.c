#include "fsdetect_impl.h"

struct ext2_super_block {
  uint32_t    s_inodes_count;
  uint32_t    s_blocks_count;
  uint32_t    s_r_blocks_count;  /* Reserved. */
  uint32_t    s_free_blocks_count;
  uint32_t    s_free_inodes_count;
  uint32_t    s_first_data_block;
  uint32_t    s_log_block_size;
  uint32_t    s_dummy3[7];
  unsigned char    s_magic[2];
  uint16_t    s_state;
  uint16_t    s_errors;
  uint16_t    s_minor_rev_level;
  uint32_t    s_lastcheck;
  uint32_t    s_checkinterval;
  uint32_t    s_creator_os;
  uint32_t    s_rev_level;
  uint16_t    s_def_resuid;
  uint16_t    s_def_resgid;
  uint32_t    s_first_ino;
  uint16_t    s_inode_size;
  uint16_t    s_block_group_nr;
  uint32_t    s_feature_compat;
  uint32_t    s_feature_incompat;
  uint32_t    s_feature_ro_compat;
  unsigned char    s_uuid[16];
  char      s_volume_name[16];
  char      s_last_mounted[64];
  uint32_t    s_algorithm_usage_bitmap;
  uint8_t      s_prealloc_blocks;
  uint8_t      s_prealloc_dir_blocks;
  uint16_t    s_reserved_gdt_blocks;
  uint8_t      s_journal_uuid[16];
  uint32_t    s_journal_inum;
  uint32_t    s_journal_dev;
  uint32_t    s_last_orphan;
  uint32_t    s_hash_seed[4];
  uint8_t      s_def_hash_version;
  uint8_t      s_jnl_backup_type;
  uint16_t    s_reserved_word_pad;
  uint32_t    s_default_mount_opts;
  uint32_t    s_first_meta_bg;
  uint32_t    s_mkfs_time;
  uint32_t    s_jnl_blocks[17];
  uint32_t    s_blocks_count_hi;
  uint32_t    s_r_blocks_count_hi;
  uint32_t    s_free_blocks_hi;
  uint16_t    s_min_extra_isize;
  uint16_t    s_want_extra_isize;
  uint32_t    s_flags;
  uint16_t    s_raid_stride;
  uint16_t    s_mmp_interval;
  uint32_t    s_mmp_block_a;
  uint32_t    s_mmp_block_b;
  uint32_t    s_raid_stripe_width;
  uint32_t    s_reserved[163];
} __attribute__((packed));

/* magic string */
#define EXT_SB_MAGIC        "\123\357"
/* supper block offset */
#define EXT_SB_OFF        0x400
/* supper block offset in kB */
#define EXT_SB_KBOFF        (EXT_SB_OFF >> 10)
/* magic string offset within super block */
#define EXT_MAG_OFF        0x38

/* for s_flags */
#define EXT2_FLAGS_TEST_FILESYS    0x0004

/* for s_feature_compat */
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL    0x0004

/* for s_feature_ro_compat */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER  0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE  0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR  0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE  0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM    0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK  0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE  0x0040

/* for s_feature_incompat */
#define EXT2_FEATURE_INCOMPAT_FILETYPE    0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER    0x0004
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV  0x0008
#define EXT2_FEATURE_INCOMPAT_META_BG    0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS    0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT    0x0080
#define EXT4_FEATURE_INCOMPAT_MMP    0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG    0x0200

#define EXT2_FEATURE_RO_COMPAT_SUPP  (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
           EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
           EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT2_FEATURE_INCOMPAT_SUPP  (EXT2_FEATURE_INCOMPAT_FILETYPE| \
           EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_INCOMPAT_UNSUPPORTED  ~EXT2_FEATURE_INCOMPAT_SUPP
#define EXT2_FEATURE_RO_COMPAT_UNSUPPORTED  ~EXT2_FEATURE_RO_COMPAT_SUPP

#define EXT3_FEATURE_RO_COMPAT_SUPP  (EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER| \
           EXT2_FEATURE_RO_COMPAT_LARGE_FILE| \
           EXT2_FEATURE_RO_COMPAT_BTREE_DIR)
#define EXT3_FEATURE_INCOMPAT_SUPP  (EXT2_FEATURE_INCOMPAT_FILETYPE| \
           EXT3_FEATURE_INCOMPAT_RECOVER| \
           EXT2_FEATURE_INCOMPAT_META_BG)
#define EXT3_FEATURE_INCOMPAT_UNSUPPORTED  ~EXT3_FEATURE_INCOMPAT_SUPP
#define EXT3_FEATURE_RO_COMPAT_UNSUPPORTED  ~EXT3_FEATURE_RO_COMPAT_SUPP

/* Code based on util-linux-2.31/libblkid/src/superblocks/ext.c */
int fsdetect_ext(read_block_t read_block, void *read_block_data,
                 struct fsdetect_output *fsdo) {
  struct ext2_super_block sb;
  uint32_t fc, fi, frc;
  if (read_block(read_block_data, 2, 1, &sb) != 0) return -1;
  /* http://www.nongnu.org/ext2-doc/ext2.html */
  if ((uint8_t)sb.s_magic[0] != 0x53 || (uint8_t)sb.s_magic[1] != 0xef
     ) return 10;
  fc = le(sb.s_feature_compat);
  fi = le(sb.s_feature_incompat);
  frc = le(sb.s_feature_ro_compat);
  if (!(fc & EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
      !((frc & EXT2_FEATURE_RO_COMPAT_UNSUPPORTED) ||
        (fi  & EXT2_FEATURE_INCOMPAT_UNSUPPORTED))) {
    strcpy(fsdo->fstype, "ext2");
  } else if ((fc & EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
             !((frc & EXT3_FEATURE_RO_COMPAT_UNSUPPORTED) ||
               (fi  & EXT3_FEATURE_INCOMPAT_UNSUPPORTED))) {
    strcpy(fsdo->fstype, "ext3");
  } else if (!(fi & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) &&
             !(!(frc & EXT3_FEATURE_RO_COMPAT_UNSUPPORTED) &&
               !(fi  & EXT3_FEATURE_INCOMPAT_UNSUPPORTED))) {
    strcpy(fsdo->fstype, "ext4");  /* Also "ext4dev". */
  } else {
    /* e.g. "jbd". */
    return 11;
  }

#ifdef DEBUG
  printf("s_magic=<%02x%02x>\n", sb.s_magic[0], sb.s_magic[1]);
  printf("s_inodes_count=0x%x\n", le32(sb.s_inodes_count));
  printf("s_blocks_count=0x%x\n", le32(sb.s_blocks_count));
  printf("s_r_blocks_count=0x%x\n", le32(sb.s_r_blocks_count));
  printf("s_free_blocks_count=0x%x\n", le32(sb.s_free_blocks_count));
  printf("s_free_inodes_count=0x%x\n", le32(sb.s_free_inodes_count));
  printf("s_first_data_block=0x%x\n", le32(sb.s_first_data_block));
  printf("s_log_block_size=0x%x\n", le32(sb.s_log_block_size));
  printf("s_state=0x%x\n", le16(sb.s_state));
  printf("s_errors=0x%x\n", le16(sb.s_errors));
  printf("s_minor_rev_level=0x%x\n", le16(sb.s_minor_rev_level));
  printf("s_lastcheck=0x%x\n", le32(sb.s_lastcheck));
  printf("s_checkinterval=0x%x\n", le32(sb.s_checkinterval));
  printf("s_creator_os=0x%x\n", le32(sb.s_creator_os));
  printf("s_rev_level=0x%x\n", le32(sb.s_rev_level));
  printf("s_def_resuid=0x%x\n", le32(sb.s_def_resuid));
  printf("s_def_resgid=0x%x\n", le16(sb.s_def_resgid));
  printf("s_first_ino=0x%x\n", le32(sb.s_first_ino));
  printf("s_inode_size=0x%x\n", le16(sb.s_inode_size));
  printf("s_block_group_nr=0x%x\n", le16(sb.s_block_group_nr));
  printf("s_feature_compat=0x%x\n", le32(sb.s_feature_compat));
  printf("s_feature_incompat=0x%x\n", le32(sb.s_feature_incompat));
  printf("s_feature_ro_compat=0x%x\n", le32(sb.s_feature_ro_compat));
  printf("s_algorithm_usage_bitmap=0x%x\n", le32(sb.s_algorithm_usage_bitmap));
  printf("s_prealloc_blocks=0x%x\n", le8(sb.s_prealloc_blocks));
  printf("s_prealloc_dir_blocks=0x%x\n", le8(sb.s_prealloc_dir_blocks));
  printf("s_reserved_gdt_blocks=0x%x\n", le16(sb.s_reserved_gdt_blocks));
  printf("s_journal_inum=0x%x\n", le32(sb.s_journal_inum));
  printf("s_journal_dev=0x%x\n", le32(sb.s_journal_dev));
  printf("s_last_orphan=0x%x\n", le32(sb.s_last_orphan));
  printf("s_def_hash_version=0x%x\n", le8(sb.s_def_hash_version));
  printf("s_jnl_backup_type=0x%x\n", le8(sb.s_jnl_backup_type));
  printf("s_reserved_word_pad=0x%x\n", le16(sb.s_reserved_word_pad));
  printf("s_default_mount_opts=0x%x\n", le32(sb.s_default_mount_opts));
  printf("s_first_meta_bg=0x%x\n", le32(sb.s_first_meta_bg));
  printf("s_mkfs_time=0x%x\n", le32(sb.s_mkfs_time));
  printf("s_blocks_count_hi=0x%x\n", le32(sb.s_blocks_count_hi));
  printf("s_r_blocks_count_hi=0x%x\n", le32(sb.s_r_blocks_count_hi));
  printf("s_free_blocks_hi=0x%x\n", le32(sb.s_free_blocks_hi));
  printf("s_min_extra_isize=0x%x\n", le16(sb.s_min_extra_isize));
  printf("s_want_extra_isize=0x%x\n", le16(sb.s_want_extra_isize));
  printf("s_flags=0x%x\n", le32(sb.s_flags));
  printf("s_raid_stride=0x%x\n", le16(sb.s_raid_stride));
  printf("s_mmp_interval=0x%x\n", le16(sb.s_mmp_interval));
  printf("s_mmp_block_a=0x%x\n", le32(sb.s_mmp_block_a));
  printf("s_mmp_block_b=0x%x\n", le32(sb.s_mmp_block_b));
  printf("s_raid_stripe_width=0x%x\n", le32(sb.s_raid_stripe_width));
#endif

  /* --- Extra checks missing from util-linux. */

  if (le(sb.s_inode_size) < 128 || !is_power_of_2(le(sb.s_inode_size))
     ) return 12;
  if (le(sb.s_inodes_count) == 0) return 13;
  /* >= is correct, an empty file system already has 1 used inode. */
  if (le(sb.s_free_inodes_count) >= le(sb.s_inodes_count)) return 14;
  if (le(sb.s_blocks_count) == 0 && le(sb.s_blocks_count_hi) == 0) return 15;
  /* TODO(pts): Detect fileystems >= 1 PiB (s_blocks_count_hi >= 0x100). */
  if (le(sb.s_blocks_count_hi) > 0xff) return 16;
  if (le(sb.s_r_blocks_count_hi) != 0) return 17;
  if (is_less_hilo(le(sb.s_blocks_count_hi), le(sb.s_blocks_count),
                   le(sb.s_r_blocks_count_hi),
                   le(sb.s_r_blocks_count))) return 18;
  /* >= is correct, an empty file system already has many used blocks. */
  if (!is_less_hilo(le(sb.s_free_blocks_hi), le(sb.s_free_blocks_count),
                    le(sb.s_blocks_count_hi), le(sb.s_blocks_count))) return 19;
  if (!is_less_hilo(0, le(sb.s_reserved_gdt_blocks),
                    le(sb.s_blocks_count_hi), le(sb.s_blocks_count))) return 20;
  if (le(sb.s_log_block_size) > 2) return 21;
  /* There is also revision 0 with fixed inode sizes, no xattr. */
  if (le(sb.s_rev_level) != 1) return 22;
  if (le(sb.s_minor_rev_level) != 0) return 23;
  if (le(sb.s_state) - 1U > 2 - 1U) return 24;  /* 1, 2 are OK. */
  if (le(sb.s_errors) - 1U > 3 - 1U) return 25;  /* 1, 2 and 3 are OK. */
  if (le(sb.s_creator_os) > 9) return 25; /* 0 .. 4 are OK. */

  memcpy(fsdo->uuid, sb.s_uuid, 16);
  fsdo->uuid_size = 16;
  strncpy(fsdo->label, sb.s_volume_name, 16);
  fsdo->label[16] = '\0';
  return 0;  /* Success. */
}

