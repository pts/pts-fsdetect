#ifndef _FSDETECT_IMPL_H
#define _FSDETECT_IMPL_H 1

#include "fsdetect.h"

/* gcc-4.8.4 has __BYTE_ORDER__, gcc-4.4.3 doesn't have it. */
#if defined(__i386) || defined(__amd64) || (__BYTE_ORDER__ && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #define le64(x) ((uint64_t)(x))
  #define le32(x) ((uint32_t)(x))
  #define le16(x) ((uint16_t)(x))
  #define le8(x) ((uint8_t)(x))
  #define le(x) ((uint32_t)(x))  /* Doesn't support uint64_t. */
#elif (__BYTE_ORDER__ && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  #define le64(x0) __extension__ ({ const uint64_t x = (x0); (uint64_t)( \
      (uint64_t)le32(x) << 32 | (uint64_t)le32(x >> 32)); }
  #define le32(x0) __extension__ ({ const uint32_t x = (x0); (uint32_t)( \
      ((x & 0x000000FF) << 24) | \
      ((x & 0x0000FF00) << 8)  | \
      ((x & 0x00FF0000) >> 8)  | \
      ((x & 0xFF000000) >> 24)); })
  #define le16(x0) __extension__ ({ const uint16_t x = (x0); (uint16_t)( \
      ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8)); })
  #define le8(x0) ((uint8_t)(x0))
  #define le(x0) ((uint32_t)( \
      sizeof(x0) == 1 ? le8(x0) : sizeof(x0) == 2 ? le16(x0) : le32(x0)))
#else  /* Unknown byte order. */
  /* TODO(pts): #define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100) */
  #error Unknown byte order.
  #define le(x) __error_missing_le__
#endif

#define is_power_of_2(x0) __extension__ ({ const __typeof__(x0) x = (x0); \
    (x & (x - 1)) == 0; })

static __inline__ char is_less_hilo(uint32_t ahi, uint32_t alo,
                                    uint32_t bhi, uint32_t blo) {
  return ahi < bhi || (ahi == bhi && alo < blo);
}

int fsdetect_ext(read_block_t read_block, void *read_block_data,
                 struct fsdetect_output *fsdo);
int fsdetect_ntfs(read_block_t read_block, void *read_block_data,
                  struct fsdetect_output *fsdo);
int fsdetect_fat(read_block_t read_block, void *read_block_data,
                 struct fsdetect_output *fsdo);

#endif /* _FSDETECT_IMPL_H */
