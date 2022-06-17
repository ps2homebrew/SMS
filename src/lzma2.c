#include <stdio.h>
#include <limits.h>

#include "lzma2.h"

static unsigned char xz_header_magic[6] = { 0xFD,'7','z','X','Z',0x00 };
static unsigned char xz_footer_magic[2] = { 'Y','Z' };

// Encodes a number into a number of bytes
size_t encode(unsigned char buf[static 9], unsigned long num)
{
	if (num > ULONG_MAX / 2)
		return 0;

	size_t i = 0;

	while (num >= 0x80)
	{
		buf[i++] = (u8)(num) | 0x80;
		num >>= 7;
	}

	buf[i++] = (u8)(num);

	return i;
}

// Returns number of bytes a number uses
size_t decode(const unsigned char buf[], size_t size_max, unsigned long *num)
{
	if (size_max == 0)
		return 0;

	if (size_max > 9)
		size_max = 9;

	*num = buf[0] & 0x7F;
	size_t i = 0;

	while (buf[i++] & 0x80)
	{
		if (i >= size_max || buf[i] == 0x00)
			return 0;

		*num |= (u64)(buf[i] & 0x7F) << (i * 7);
	}

	return i;
}

size_t lzma2_decode_size(unsigned char *index)
{
	int i;
	unsigned long records;
	unsigned long size;
	size_t uncompressed_size = 0;
	unsigned char *buf = index;

	if (*buf != 0)
	{
		return 0;
	}

	// First byte is 0
	buf++;

	// Number of records
	buf += decode(buf,9,&records);

	for (i = 0; i < records; i++)
	{
		// Unpadded size
		buf += decode(buf,9,&size);
		size = 0L;
		// Uncompressed size
		buf += decode(buf,9,&size);
		uncompressed_size += size;
	}

	return uncompressed_size;

}

size_t lzma2_get_uncompressed_size(unsigned char *buf, u64           size)
{

	int i;
	unsigned char *index;
	xz_header_t *header;
	xz_footer_t *footer;

	if (buf == NULL)
	{
		return 0;
	}

	header = (xz_header_t*)buf;
	footer = (xz_footer_t*)(buf + size - 12);

	for (i = 0; i < 6; i++)
	{
		if (header->magic[i] != xz_header_magic[i])
		{
			return 0;
		}
	}

	for (i = 0; i < 2; i++)
	{
		if (footer->magic[i] != xz_footer_magic[i])
		{
			return 0;
		}
	}

	i = footer->backward_size;

	index = (unsigned char*)footer - ((i + 1)*4);

	return lzma2_decode_size(index);

}

enum xz_ret lzma2_uncompress(unsigned char *in, size_t size_in, unsigned char *out, size_t size_out)
{

	int ret;
	struct xz_buf buf;
	struct xz_dec *dec;

	xz_crc32_init();

	buf.in = in;
	buf.in_pos = 0;
	buf.in_size = size_in;

	buf.out = out;
	buf.out_pos = 0;
	buf.out_size = size_out;

	// Single call mode ignores second arg
	dec = xz_dec_init(XZ_SINGLE,0);

	if (dec == NULL)
	{
		return XZ_MEMLIMIT_ERROR;
	}

	ret = xz_dec_run(dec,&buf);

	xz_dec_end(dec);

	return ret;

}
