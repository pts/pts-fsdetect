#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "fsdetect_impl.h"

struct ntfs_bios_parameters {
  uint16_t  sector_size;  /* Size of a sector in bytes. */
  uint8_t   sectors_per_cluster;  /* Size of a cluster in sectors. */
  uint16_t  reserved_sectors;  /* zero */
  uint8_t   fats;      /* zero */
  uint16_t  root_entries;    /* zero */
  uint16_t  sectors;    /* zero */
  uint8_t   media_type;    /* 0xf8 = hard disk */
  uint16_t  sectors_per_fat;  /* zero */
  uint16_t  sectors_per_track;  /* irrelevant */
  uint16_t  heads;      /* irrelevant */
  uint32_t  hidden_sectors;    /* zero */
  uint32_t  large_sectors;    /* zero */
} __attribute__ ((__packed__));

struct ntfs_super_block {
  uint8_t    jump[3];
  uint8_t    oem_id[8];  /* magic string */

  struct ntfs_bios_parameters  bpb;

  uint16_t  unused[2];
  uint64_t  number_of_sectors;
  uint64_t  mft_cluster_location;
  uint64_t  mft_mirror_cluster_location;
  int8_t    clusters_per_mft_record;
  uint8_t   reserved1[3];
  int8_t    cluster_per_index_record;
  uint8_t   reserved2[3];
  uint64_t  volume_serial;
  uint32_t  checksum;
  char      padding[428];  /* Pad it to 512 bytes. */
} __attribute__((packed));

struct master_file_table_record {
  uint32_t  magic;
  uint16_t  usa_ofs;
  uint16_t  usa_count;
  uint64_t  lsn;
  uint16_t  sequence_number;
  uint16_t  link_count;
  uint16_t  attrs_offset;
  uint16_t  flags;
  uint32_t  bytes_in_use;
  uint32_t  bytes_allocated;
} __attribute__((__packed__));

struct file_attribute {
  uint32_t  type;
  uint32_t  len;
  uint8_t   non_resident;
  uint8_t   name_len;
  uint16_t  name_offset;
  uint16_t  flags;
  uint16_t  instance;
  uint32_t  value_len;
  uint16_t  value_offset;
} __attribute__((__packed__));

#define MFT_RECORD_VOLUME  3
#define NTFS_MAX_CLUSTER_SIZE  (64 * 1024)

#define MFT_RECORD_ATTR_VOLUME_NAME 0x60U
#define MFT_RECORD_ATTR_END 0xffffffffU

/* Code based on util-linux-2.31/libblkid/src/superblocks/ntfs.c */
int fsdetect_ntfs(read_block_t read_block, void *read_block_data,
                  struct fsdetect_output *fsdo) {
  struct ntfs_super_block sb;
  unsigned char buf[4096];
  struct master_file_table_record *mft;
  uint32_t sectors_per_cluster, mft_record_size;
  uint16_t sector_size;
  uint32_t block_off, attr_off;
  uint64_t nr_clusters;

  if (read_block(read_block_data, 0, 1, &sb) != 0) return -1;
  if (0 != memcmp(sb.oem_id, "NTFS    ", 8) &&
      0 != memcmp(sb.oem_id, "MSWIN4.0", 8) &&
      0 != memcmp(sb.oem_id, "MSWIN4.1", 8)
     ) return 10;

  /*
   * Check bios parameters block
   */
  sector_size = le16(sb.bpb.sector_size);
  sectors_per_cluster = sb.bpb.sectors_per_cluster;

  /* This is more strict than util-linux. */
  switch (sector_size) {
   case 512: case 1024: case 2048: case 4096:
    break;
   default:
    return 11;
  }

  switch (sectors_per_cluster) {
   case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128:
    break;
   default:
    return 12;
  }

  if ((uint16_t) le(sb.bpb.sector_size) *
      sb.bpb.sectors_per_cluster > NTFS_MAX_CLUSTER_SIZE)
    return 13;

  /* Unused fields must be zero */
  if (le(sb.bpb.reserved_sectors)
      || le(sb.bpb.root_entries)
      || le(sb.bpb.sectors)
      || le(sb.bpb.sectors_per_fat)
      || le(sb.bpb.large_sectors)
      || sb.bpb.fats)
    return 14;

  if (!(0xf8 <= sb.bpb.media_type || sb.bpb.media_type == 0xf0))
    return 22;

  if ((uint8_t) sb.clusters_per_mft_record < 0xe1
      || (uint8_t) sb.clusters_per_mft_record > 0xf7) {

    switch (sb.clusters_per_mft_record) {
    case 1: case 2: case 4: case 8: case 16: case 32: case 64:
      break;
    default:
      return 15;
    }
  }

  if (sb.clusters_per_mft_record > 0)
    mft_record_size = sb.clusters_per_mft_record *
          sectors_per_cluster * sector_size;
  else
    mft_record_size = 1 << (0 - sb.clusters_per_mft_record);
  switch (mft_record_size) {
   /* Maximum is 4096 so that it fits to buf, and also the NTFS kernel
    * driver needs it to be at most the page size.
    */
   case 512: case 1024: case 2048: case 4096:
    break;
   default:
    return 21;
  }

  nr_clusters = le64(sb.number_of_sectors) / sectors_per_cluster;

  if ((le64(sb.mft_cluster_location) >= nr_clusters) ||
      (le64(sb.mft_mirror_cluster_location) >= nr_clusters))
    return 16;


  block_off = le64(sb.mft_cluster_location) * (sector_size >> 9) *
      sectors_per_cluster;

  /* Typical: mft_record_size=1024 */
#ifdef DEBUG
  __extension__ fprintf(stderr,
      "NTFS: sector_size=%u, mft_record_size=%u, "
      "sectors_per_cluster=%u, nr_clusters=%llu "
      "cluster_block_offset=%llu\n",
      sector_size, mft_record_size,
      sectors_per_cluster, (long long)nr_clusters,
      (long long)block_off);
#endif

  if (read_block(read_block_data, block_off, mft_record_size >> 9, buf) != 0) return 17;
  if (memcmp(buf, "FILE", 4)) return 18;
  block_off += MFT_RECORD_VOLUME * (mft_record_size >> 9);
  if (read_block(read_block_data, block_off, mft_record_size >> 9, buf) != 0) return 19;
  if (memcmp(buf, "FILE", 4)) return 20;

  strcpy(fsdo->fstype, "ntfs");
  mft = (struct master_file_table_record *) buf;
  attr_off = le(mft->attrs_offset);
  while (attr_off + sizeof(struct file_attribute) <= mft_record_size &&
         attr_off <= le(mft->bytes_allocated)) {

    uint32_t attr_len;
    struct file_attribute *attr;

    attr = (struct file_attribute *) (buf + attr_off);
    attr_len = le(attr->len);
    if (!attr_len)
      break;

    if (le(attr->type) == MFT_RECORD_ATTR_END)
      break;
    if (le(attr->type) == MFT_RECORD_ATTR_VOLUME_NAME) {
      unsigned int val_off = le(attr->value_offset);
      unsigned int val_len = le(attr->value_len);
      unsigned char *val = ((uint8_t *) attr) + val_off;

      if (attr_off + val_off + val_len <= mft_record_size) {
        /* TODO(pts): Smarter. */
        char *p = fsdo->label;
        if (val_len > 2 * sizeof(fsdo->label) - 2) {
          val_len = 2 * sizeof(fsdo->label) - 2;
        }
        for (; val_len > 1; val_len -= 2, val += 2) {
          if (val[1] == 0 && val[0] <= 0x7f) {
            *p++ = val[0];  /* ASCII */
          } else {
            *p++ = '?';
          }
        }
      }
      break;
    }

    attr_off += attr_len;
  }

  fsdo->uuid_size = 8;
#if defined(__i386) || defined(__amd64)
  {
    char *p = (char*)fsdo->uuid;
    *(uint64_t*)p = le64(sb.volume_serial);
  }
#else
  {
    const uint64_t s = le64(sb.volume_serial);
    unsigned u;
    uint8_t *p = fsdo->uuid + 7;
    for (u = 0; u < 64; u += 8) {
      *p-- = s >> u;
    }
  }
#endif
  return 0;
}
