#ifndef __LZMA2_H__
#define __LZMA2_H__

#include "xz.h"

typedef struct
{
    unsigned char magic[6];
    unsigned char flags[2];
    unsigned int crc32;
} xz_header_t __attribute__((packed));

typedef struct
{
    unsigned int crc32;
    unsigned int backward_size; // real_backward_size = (stored + 1)*4
    unsigned char flags[2];     //identical to header
    unsigned char magic[2];
} xz_footer_t __attribute__((packed));

typedef struct
{
    void *in;
    int in_size;
    void *out;
    int out_size;
} lzma2_pkt_t;

#ifdef __cplusplus
extern "C" {
#endif

size_t lzma2_get_uncompressed_size(unsigned char *buf, u64           size);
enum xz_ret lzma2_uncompress(unsigned char *in, size_t size_in, unsigned char *out, size_t size_out);

#ifdef __cplusplus
};
#endif

#endif /*__LZMA2_H__*/
