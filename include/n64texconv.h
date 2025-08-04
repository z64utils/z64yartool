/* n64texconv.h -- imported from z64convert, with some tweaks */

#ifndef N64TEXCONV_H_INCLUDED
#define N64TEXCONV_H_INCLUDED

#include <stdlib.h> /* size_t */

struct n64texconv_palctx;

enum n64texconv_fmt
{
	N64TEXCONV_RGBA = 0 /* same order as gbi.h, important! */
	, N64TEXCONV_YUV
	, N64TEXCONV_CI
	, N64TEXCONV_IA
	, N64TEXCONV_I
	, N64TEXCONV_1BIT
	, N64TEXCONV_FMT_MAX
};


enum n64texconv_bpp
{
	N64TEXCONV_4 = 0 /* same order as gbi.h, important! */
	, N64TEXCONV_8
	, N64TEXCONV_16
	, N64TEXCONV_32
};


/* convert N64 texture data to RGBA8888
 * returns 0 (NULL) on success, pointer to error string otherwise
 * error string will be returned if...
 * * invalid fmt/bpp combination is provided
 * * color-indexed format is provided, yet `pal` is 0 (NULL)
 * `pal` can be 0 (NULL) if format is not color-indexed
 * `dst` must point to data you have already allocated
 * `pix` must point to pixel data
 * `w` and `h` must > 0
 * NOTE: `dst` and `pix` can be the same to convert in-place, but
 *       you must ensure the buffer is large enough for the result
 */
const char *
n64texconv_to_rgba8888(
	unsigned char *dst
	, unsigned char *pix
	, unsigned char *pal
	, enum n64texconv_fmt fmt
	, enum n64texconv_bpp bpp
	, int w
	, int h
);


/* convert RGBA8888 to N64 texture data
 * returns 0 (NULL) on success, pointer to error string otherwise
 * error string will be returned if...
 * * invalid fmt/bpp combination is provided
 * * color-indexed format is provided (conversion to ci is unsupported)
 * `dst` must point to data you have already allocated
 * `pix` must point to pixel data
 * `w` and `h` must be > 0
 * NOTE: `dst` and `pix` can be the same to convert in-place, but
 *       you must ensure the buffer is large enough for the result
 */
const char *
n64texconv_to_n64(
	unsigned char *dst
	, unsigned char *pix
	, unsigned char *pal
	, int pal_colors
	, enum n64texconv_fmt fmt
	, enum n64texconv_bpp bpp
	, int w
	, int h
	, unsigned int *sz
);


const char *
n64texconv_to_n64_and_back(
	unsigned char *pix
	, unsigned char *pal
	, int pal_colors
	, enum n64texconv_fmt fmt
	, enum n64texconv_bpp bpp
	, int w
	, int h
);

#endif /* N64TEXCONV_H_INCLUDED */

