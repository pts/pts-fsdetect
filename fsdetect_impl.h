#ifndef _FSDETECT_IMPL_H
#define _FSDETECT_IMPL_H 1

#include "fsdetect.h"

#ifdef __XTINY__
#include <xtiny.h>
#ifdef DEBUG
#error __XTINY__ does not support DEBUG.
#endif
#else
#ifdef __TINYC__
typedef unsigned int size_t;
/* Doesn't seem to be needed:
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned long uint32_t;
typedef long int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
*/
extern void *memset(void *__s, int __c, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1)));
extern void *memcpy(void *__restrict __dest, __const void *__restrict __src, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strcpy(char *__restrict __dest, __const char *__restrict __src) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *strncpy(char *__restrict __dest, __const char *__restrict __src, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__nonnull__ (1, 2)));
extern int memcmp (__const void *__s1, __const void *__s2, size_t __n) __attribute__ ((__nothrow__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
#ifdef DEBUG
extern int printf (__const char *__restrict __format, ...);
#endif
#else
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#endif
#endif

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

#if defined(__i386) || defined(__amd64)
  #define unaligned_le16(x) __extension__ ({ char *p = (char*)&(x); *(uint16_t*)p; })
#else
  #define unaligned_le16(x) \
      (((unsigned char*)x)[0] + (((unsigned char*)x)[1] << 8))
#endif

#define is_power_of_2(x0) __extension__ ({ const __typeof__(x0) x = (x0); \
    (x != 0) && (x & (x - 1)) == 0; })

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
int fsdetect_btrfs(read_block_t read_block, void *read_block_data,
                   struct fsdetect_output *fsdo);

#endif /* _FSDETECT_IMPL_H */
