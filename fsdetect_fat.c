#include "fsdetect_impl.h"

#if defined(__TINYC__)
#pragma pack(push, 1)
#endif

/* https://en.wikipedia.org/wiki/BIOS_parameter_block
 *
 * We don't detect very old versions of FAT such as those created by MS-DOS
 * earlier than 4.0.
 *
 * We don't use the term VFAT, because it's related to long filenames, about
 * which we don't care here.
 */
struct fat_super_block {  /* 512 bytes. */
  /* 00*/  unsigned char  ms_jump[3];
  /* 03*/  unsigned char  ms_sysid[8];
  /* 0b*/  uint16_t   ms_sector_size;  /* Unaligned. */
  /* 0d*/  uint8_t    ms_cluster_size;
  /* 0e*/  uint16_t   ms_reserved;
  /* 10*/  uint8_t    ms_fats;
  /* 11*/  uint16_t   ms_root_entries;  /* Unaligned. */
  /* 13*/  uint16_t   ms_sectors; /* Unaligned. =0 iff V3 or later */
  /* 15*/  unsigned char  ms_media;
  /* 16*/  uint16_t  ms_fat_length; /* Sectors per FAT */
  /* 18*/  uint16_t  ms_secs_track;
  /* 1a*/  uint16_t  ms_heads;
  /* 1c*/  uint32_t  ms_hidden;
  /* V3 BPB */
  /* 20*/  uint32_t  ms_total_sect; /* iff ms_sectors == 0 */
  union fat {
    struct f12 {  /* For FAT12 and FAT16. */
      /* V4 BPB */
      /* 24*/  uint16_t physical_drive_number;
      /* 26*/  uint8_t signature;
      /* 27*/  unsigned char serno[4];
      /* 2b*/  unsigned char label[11];
      /* 36*/  unsigned char magic[8];
      /* 3e*/  unsigned char dummy2[0x1fe - 0x3e];
    } __attribute__((packed)) f1x;
    struct {  /* For FAT32. */
      /* 24*/  uint32_t  fat32_length;
      /* 28*/  uint16_t  flags;
      /* 2a*/  uint16_t  version;
      /* 2c*/  uint32_t  root_cluster;
      /* 30*/  uint16_t  fsinfo_sector;
      /* 32*/  uint16_t  backup_boot;
      /* 34*/  uint8_t   boot_file_name[12];
      /* 40*/  uint8_t   drive_number;
      /* 41*/  uint8_t   flags2;
      /* 42*/  uint8_t   signature;
      /* 43*/  unsigned char serno[4];
      /* 47*/  unsigned char label[11];
      /* 52*/  unsigned char magic[8];
      /* 5a*/  unsigned char dummy2[0x1fe - 0x5a];
    } __attribute__((packed)) f32;
  } fat;
  /*1fe*/ uint16_t ms_boot_signature;
} __attribute__((packed));

#if defined(__TINYC__)
#pragma pack(pop)
#endif

struct Assert512BytesStruct {
   int Assert512Bytes : sizeof(struct fat_super_block) == 512; };

struct fat32_fsinfo {
  uint8_t signature1[4];
  uint32_t reserved1[120];
  uint8_t signature2[4];
  uint32_t free_clusters;
  uint32_t next_cluster;
  uint32_t reserved2[4];
} __attribute__((packed));

struct AssertFsi512BytesStruct {
     int AssertFsi512Bytes : sizeof(struct fat32_fsinfo) == 512; };

/* maximum number of clusters */
#define FAT12_MAX 0xFF4
#define FAT16_MAX 0xFFF4
#define FAT32_MAX 0x0FFFFFF6

#define FAT_ATTR_VOLUME_ID    0x08
#define FAT_ATTR_DIR      0x10
#define FAT_ATTR_LONG_NAME    0x0f
#define FAT_ATTR_MASK      0x3f
#define FAT_ENTRY_FREE      0xe5

static const char no_name[] = "NO NAME    ";

int fsdetect_fat(read_block_t read_block, void *read_block_data,
                 struct fsdetect_output *fsdo) {
  struct fat_super_block sb;
  uint16_t sector_size, dir_entries, reserved;
  uint32_t sect_count, fat_size, dir_size, cluster_count, fat_length;
  uint32_t max_count;
  uint8_t fat_bits;
  uint16_t fsinfo_sect;
  const unsigned char *vol_label = 0;
  unsigned char *vol_serno = 0;

  if (read_block(read_block_data, 0, 1, &sb) != 0) return 10;
  if (!(sb.ms_jump[0] == (unsigned char)'\xeb' && sb.ms_jump[2] == (unsigned char)'\x90') &&
      !(sb.ms_jump[0] == (unsigned char)'\xe9' && sb.ms_jump[2] <= 1)) return 11;
  if ((0 != memcmp(sb.fat.f32.magic, "FAT32   ", 8) ||
       0 != memcmp(sb.fat.f32.magic, "MSWIN4.0", 8) ||
       0 != memcmp(sb.fat.f32.magic, "MSWIN4.1", 8)) &&
      sb.fat.f32.signature == 0x29  /* can be 0x28 */
     ) {
    fat_bits = 32;
    strcpy(fsdo->fstype, "fat32");
    max_count = FAT32_MAX;
  } else if (sb.fat.f1x.signature != 0x29) {  /* MS-DOS >=4.0 */
    return 12;
  } else if (0 == memcmp(sb.fat.f1x.magic, "FAT12   ", 8)) {
    fat_bits = 12;
    strcpy(fsdo->fstype, "fat12");
    max_count = FAT12_MAX;
  } else if (0 == memcmp(sb.fat.f1x.magic, "FAT16   ", 8)) {
    fat_bits = 16;
    strcpy(fsdo->fstype, "fat16");
    max_count = FAT16_MAX;
  } else if (0 == memcmp(sb.fat.f1x.magic, "MSDOS   ", 8)) {
    fat_bits = 126;  /* We'll figure it out later. */
    strcpy(fsdo->fstype, "fat12");
    max_count = FAT12_MAX;
  } else {
    return 13;
  }
  reserved = le16(sb.ms_reserved);
  dir_entries = unaligned_le16(sb.ms_root_entries);
  sect_count = unaligned_le16(sb.ms_sectors);
  if (sect_count == 0)
    sect_count = le32(sb.ms_total_sect);

  if (sb.ms_fats - 1U > 2 - 1U)  /* NTFS has 0 here. */
    return 14;
  if (!reserved)  /* NTFS has 0 here. */
    return 15;
  if (!(0xf8 <= sb.ms_media || sb.ms_media == 0xf0))
    return 16;
  if (!is_power_of_2(sb.ms_cluster_size))
    return 17;
  if (fat_bits != 32 && dir_entries == 0)
    return 31;
  if (le16(sb.ms_boot_signature) != 0xaa55)
    return 33;

  sector_size = unaligned_le16(sb.ms_sector_size);
  switch (sector_size) {
   case 512: case 1024: case 2048: case 4096:
   case 8192: case 16384: case 32768:
    break;
   default:
    return 18;
  }
/* Don't check, mkfs.vfat can create a different one.
  if (fat_bits != 32 && (dir_entries * 32 % sector_size) != 0)
    return 32;
*/

  fat_length = le16(sb.ms_fat_length);
  if (fat_length == 0) {
    if (fat_bits != 32) return 19;
    fat_length = le32(sb.fat.f32.fat32_length);
  }

  fat_size = fat_length * sb.ms_fats;
  dir_size = ((dir_entries * 32) +
          (sector_size-1)) / sector_size;

  cluster_count = sect_count - (reserved + fat_size + dir_size);
  if ((int32_t)cluster_count <= 0) return 24;
#if 0  /* TODO(pts): Are there any unused sectors in the end? */
  if (cluster_count % sb.ms_cluster_size != 0) return 25;
#endif
  cluster_count /= sb.ms_cluster_size;
  if (fat_bits == 126) {
    if (cluster_count > FAT12_MAX) {
      fat_bits = 16;
      strcpy(fsdo->fstype, "fat16");
      max_count = FAT16_MAX;
    } else {
      fat_bits = 12;
      strcpy(fsdo->fstype, "fat12");
      max_count = FAT12_MAX;
    }
  }
  if (cluster_count > max_count) return 20;

  if (fat_bits != 32) {
    /* There is also a label in the root directory, but we use the one in
     * the boot sector, for simplicity.
     */
    vol_label = sb.fat.f1x.label;
    vol_serno = sb.fat.f1x.serno;
  } else {
    vol_label = sb.fat.f32.label;
    vol_serno = sb.fat.f32.serno;
    /*
     * FAT32 should have a valid signature in the fsinfo block,
     * but also allow all bytes set to '\0', because some volumes
     * do not set the signature at all.
     */
    fsinfo_sect = le16(sb.fat.f32.fsinfo_sector);
    if (fsinfo_sect) {
      struct fat32_fsinfo fsinfo;
      if (read_block(read_block_data, fsinfo_sect * (sector_size >> 9), 1, &fsinfo) != 0) return 21;
      /* buggy mkfs.vfat -C creates a copy of the boot sector here. */
      if (0 != memcmp(fsinfo.signature1, "\xeb\x58\x90", 3)) {
        if (memcmp(fsinfo.signature1, "\x52\x52\x61\x41", 4) != 0 &&
            memcmp(fsinfo.signature1, "\x52\x52\x64\x41", 4) != 0 &&
            memcmp(fsinfo.signature1, "\x00\x00\x00\x00", 4) != 0)
          return 22;
        if (memcmp(fsinfo.signature2, "\x72\x72\x41\x61", 4) != 0 &&
            memcmp(fsinfo.signature2, "\x00\x00\x00\x00", 4) != 0)
          return 23;
      }
    }
  }

  if (vol_label && memcmp(vol_label, no_name, 11) != 0) {
    char *p = fsdo->label, *q = p + 11;
    memcpy(p, vol_label, 11);
    for (; p != q && q[-1] == ' '; --q) {}
    q[0] = '\0';
  }

  fsdo->uuid_size = 4;
  fsdo->uuid[3] = *vol_serno++;  /* Same order as mdir and blkid. */
  fsdo->uuid[2] = *vol_serno++;
  fsdo->uuid[1] = *vol_serno++;
  fsdo->uuid[0] = *vol_serno++;
  return 0;
}
