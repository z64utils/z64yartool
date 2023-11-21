/* n64texconv.c -- imported from z64convert, with some tweaks */

#include <math.h>

#include "n64texconv.h"

struct vec4b {
	unsigned char x;
	unsigned char y;
	unsigned char z;
	unsigned char w;
};

struct vec4b_2n64 {
	unsigned char x;
	unsigned char y;
	unsigned char z;
	unsigned char w;
};

#define CONV_7(x)   lut_7[x]
#define CONV_15(x)  lut_15[x]
#define CONV_31(x)  lut_31[x]
#define CONV_255(x) x

static const unsigned char lut_7[] =
{
	0, 36, 73, 109, 146, 182, 219, 255
};

static const unsigned char lut_15[] =
{
	0, 17, 34, 51, 68, 85, 102, 119,
	136, 153, 170, 187, 204, 221, 238, 255
};

static const unsigned char lut_31[] =
{
	0, 8, 16, 25, 33, 41, 49, 58, 66,
	74, 82, 90, 99, 107, 115, 123, 132,
	140, 148, 156, 165, 173, 181, 189,
	197, 206, 214, 222, 230, 239, 247, 255
};

static
inline
int
imn(int a, int b)
{
	return (a < b) ? a : b;
}

static
inline
int
imx(int a, int b)
{
	return (a > b) ? a : b;
}

#define N64_COLOR_FUNC_NAME_TO(FMT) v4b_to_##FMT

#define N64_COLOR_FUNC_NAME(FMT) v4b_from_##FMT

#define N64_COLOR_FUNC(FMT) \
static \
inline \
void \
N64_COLOR_FUNC_NAME(FMT)( \
	struct vec4b *color \
	, unsigned char *b \
)

#define N64_COLOR_FUNC_TO(FMT) \
static \
inline \
void \
N64_COLOR_FUNC_NAME_TO(FMT)( \
	struct vec4b_2n64 *color \
	, unsigned char *b \
)

/* from formats */
N64_COLOR_FUNC(i4)
{
	color->x = CONV_15(*b);
	color->y = color->x;
	color->z = color->x;
	color->w = color->x;// * 0.5; /* TODO: * 0.5 fixes kokiri path */
}


N64_COLOR_FUNC(ia4)
{
	color->x = CONV_7(*b >> 1);
	color->y = color->x;
	color->z = color->x;
	color->w = ((*b) & 1) * 255;
}


N64_COLOR_FUNC(i8)
{
	color->x = CONV_255(*b);
	color->y = color->x;
	color->z = color->x;
	color->w = color->x;
}


N64_COLOR_FUNC(ia8)
{
	color->x = CONV_15(*b >> 4);
	color->y = color->x;
	color->z = color->x;
	color->w = CONV_15(*b & 0xF);
}


N64_COLOR_FUNC(ia16)
{
	color->x = CONV_255(*b);
	color->y = color->x;
	color->z = color->x;
	color->w = CONV_255(b[1]);
}


N64_COLOR_FUNC(rgba5551)
{
	int comb = (b[0]<<8)|b[1];
	color->x = CONV_31((comb >> 11) & 0x1F);
	color->y = CONV_31((comb >>  6) & 0x1F);
	color->z = CONV_31((comb >>  1) & 0x1F);
	color->w = (comb & 1) * 255;
}


N64_COLOR_FUNC(ci8)
{
	/* note that this is just rgba5551 */
	int comb = (b[0]<<8)|b[1];
	color->x = CONV_31((comb >> 11) & 0x1F);
	color->y = CONV_31((comb >>  6) & 0x1F);
	color->z = CONV_31((comb >>  1) & 0x1F);
	color->w = (comb & 1) * 255;
}


N64_COLOR_FUNC(ci4)
{
	/* note that this is just rgba5551 */
	int comb = (b[0]<<8)|b[1];
	color->x = CONV_31((comb >> 11) & 0x1F);
	color->y = CONV_31((comb >>  6) & 0x1F);
	color->z = CONV_31((comb >>  1) & 0x1F);
	color->w = (comb & 1) * 255;
}


N64_COLOR_FUNC(rgba8888)
{
	color->x = CONV_255(b[0]);
	color->y = CONV_255(b[1]);
	color->z = CONV_255(b[2]);
	color->w = CONV_255(b[3]);
}

/* 1bit */
N64_COLOR_FUNC(onebit)
{
	int i = 8;
	for (i = 0; i < 8; ++i)
	{
		if (((*b)>>i)&1)
		{
			color->x = 0xFF;
			color->y = 0xFF;
			color->z = 0xFF;
			color->w = 0xFF;
		}
		else
		{
			color->x = 0;
			color->y = 0;
			color->z = 0;
			color->w = 0xFF;
		}
		--color;
	}
}

/**************************************************************/

/* to formats */
N64_COLOR_FUNC_TO(i4)
{
	/* TUNING could omit float math altogether and just use (x & 15) */
	float f = color->x * 0.003921569f;
	int i;
	f *= 15;
	i = roundf(f);
	*b = i;
}


N64_COLOR_FUNC_TO(ia4)
{
	/* TUNING could omit float math altogether and just use (x & 7) */
	float f = color->x * 0.003921569f;
	int i;
	f *= 7;
	i = roundf(f);
	*b = (i << 1) | ((color->w & 0x80) > 1);
//	*b = (color->x & 0xFE) | ((color->w & 0x80) > 1);
}


N64_COLOR_FUNC_TO(i8)
{
	*b = color->x;
}


N64_COLOR_FUNC_TO(ia8)
{
	/* TUNING could omit float math altogether and just use (x << 4) */
	float fX = color->x * 0.003921569f;
	float fW = color->w * 0.003921569f;
	int iX;
	int iW;
	fX *= 15;
	iX = roundf(fX);
	fW *= 15;
	iW = roundf(fW);
//	*b = (iX << 4) | (color->w & 0xF);
	*b = (iX << 4) | (iW);
}


N64_COLOR_FUNC_TO(ia16)
{
	b[0] = color->x;
	b[1] = color->w;
}

N64_COLOR_FUNC_TO(rgba5551)
{
	int comb = 0;
	
	comb |= (color->x & 0xF8) << 8; // r
	comb |= (color->y & 0xF8) << 3; // g
	comb |= (color->z & 0xF8) >> 2; // b
	comb |= (color->w & 0x80)  > 1; // a
	
	b[0] = comb >> 8;
	b[1] = comb;
}


N64_COLOR_FUNC_TO(ci8)
{
	/* TODO ci conversion */
	(void)color;
	(void)b;
}


N64_COLOR_FUNC_TO(ci4)
{
	/* TODO ci conversion */
	(void)color;
	(void)b;
}


N64_COLOR_FUNC_TO(rgba8888)
{
	b[0] = CONV_255(color->x);
	b[1] = CONV_255(color->y);
	b[2] = CONV_255(color->z);
	b[3] = CONV_255(color->w);
}


static
inline
void
vec4b_2n64_from_rgba5551(struct vec4b_2n64 *color, unsigned char b[2])
{
	int comb = (b[0]<<8)|b[1];
	color->x = CONV_31((comb >> 11) & 0x1F);
	color->y = CONV_31((comb >>  6) & 0x1F);
	color->z = CONV_31((comb >>  1) & 0x1F);
	color->w = (comb & 1) * 255;
}


/* converter array */
static
void
(*n64_colorfunc_array[])(
	struct vec4b *color
	, unsigned char *b
) = {
	/* rgba = 0 */
	0,                             /*  4b */
	0,                             /*  8b */
	N64_COLOR_FUNC_NAME(rgba5551), /* 16b */
	N64_COLOR_FUNC_NAME(rgba8888), /* 32b */
	
	/* yuv = 1 */
	0,                             /*  4b */
	0,                             /*  8b */
	0,                             /* 16b */
	0,                             /* 32b */
	
	/* ci = 2 */
	N64_COLOR_FUNC_NAME(ci4),      /*  4b */
	N64_COLOR_FUNC_NAME(ci8),      /*  8b */
	0,                             /* 16b */
	0,                             /* 32b */
	
	/* ia = 3 */
	N64_COLOR_FUNC_NAME(ia4),      /*  4b */
	N64_COLOR_FUNC_NAME(ia8),      /*  8b */
	N64_COLOR_FUNC_NAME(ia16),     /* 16b */
	0,                             /* 32b */
	
	/* i = 4 */
	N64_COLOR_FUNC_NAME(i4),       /*  4b */
	N64_COLOR_FUNC_NAME(i8),       /*  8b */
	0,                             /* 16b */
	0,                             /* 32b */
	
	/* 1bit = 5 */
	N64_COLOR_FUNC_NAME(onebit),   /*  4b */
	N64_COLOR_FUNC_NAME(onebit),   /*  8b */
	N64_COLOR_FUNC_NAME(onebit),   /* 16b */
	N64_COLOR_FUNC_NAME(onebit)    /* 32b */
};


/* converter array (to n64 format) */
static
void
(*n64_colorfunc_array_to[])(
	struct vec4b_2n64 *color
	, unsigned char *b
) = {
	/* rgba = 0 */
	0,                                /*  4b */
	0,                                /*  8b */
	N64_COLOR_FUNC_NAME_TO(rgba5551), /* 16b */
	N64_COLOR_FUNC_NAME_TO(rgba8888), /* 32b */
	
	/* yuv = 1 */
	0,                                /*  4b */
	0,                                /*  8b */
	0,                                /* 16b */
	0,                                /* 32b */
	
	/* ci = 2 */
	N64_COLOR_FUNC_NAME_TO(ci4),      /*  4b */
	N64_COLOR_FUNC_NAME_TO(ci8),      /*  8b */
	0,                                /* 16b */
	0,                                /* 32b */
	
	/* ia = 3 */
	N64_COLOR_FUNC_NAME_TO(ia4),      /*  4b */
	N64_COLOR_FUNC_NAME_TO(ia8),      /*  8b */
	N64_COLOR_FUNC_NAME_TO(ia16),     /* 16b */
	0,                                /* 32b */
	
	/* i = 4 */
	N64_COLOR_FUNC_NAME_TO(i4),       /*  4b */
	N64_COLOR_FUNC_NAME_TO(i8),       /*  8b */
	0,                                /* 16b */
	0                                 /* 32b */
};


static
inline
int
diff_int(int a, int b)
{
	if (a < b)
		return b - a;
	
	return a - b;
}


static
inline
unsigned int
get_size_bytes(int w, int h, int is_1bit, enum n64texconv_bpp bpp)
{
	/* 1bit per pixel */
	if (is_1bit)
		return (w * h) / 8;
	
	switch (bpp)
	{
		case N64TEXCONV_4:  return (w * h) / 2;
		case N64TEXCONV_8:  return w * h;
		case N64TEXCONV_16: return w * h * 2;
		case N64TEXCONV_32: return w * h * 4;
	}
	
	/* 1/2 byte per pixel */
	if (bpp == N64TEXCONV_4)
		return (w * h) / 2;
	
	/* 4 bytes per pixel */
	if (bpp == N64TEXCONV_32)
		return w * h * 4;
	
	/* 1 or 2 bytes per pixel */
	return w * h * bpp;
}


static
inline
void
texture_to_rgba8888(
	void n64_colorfunc(struct vec4b *color, unsigned char *b)
	, unsigned char *dst
	, unsigned char *pix
	, unsigned char *pal
	, int is_ci
	, enum n64texconv_bpp bpp
	, int w
	, int h
)
{
	struct vec4b *color = (struct vec4b*)dst;
	int is_4bit = (bpp == N64TEXCONV_4);
	int alt = 1;
	int i;
	unsigned int sz;
	int is_1bit = 0;//(n64_colorfunc == N64_COLOR_FUNC_NAME(onebit));
	
	/* determine resulting size */
	sz = get_size_bytes(w, h, is_1bit, bpp);
	
	/* work backwards from end */
	color += w * h;
	pix += sz;
	
#if 0
	if (is_1bit)
	{
		/* this converter operates on 8 px at a time */
		/* start with last pixel */
		--color;
		for (i=0; i < w * h; i += 8)
		{
			/* pixel advancement logic */
			--pix;
			
			/* convert pixel */
			n64_colorfunc(color, pix);
			color -= 8;
		}
		return;
	}
#endif
	
	for (i=0; i < w * h; ++i)
	{
		/* pixel advancement logic */
		/* 4bpp */
		if (is_4bit)
			pix -= alt & 1;
		/* 32bpp */
		else if (bpp == N64TEXCONV_32)
			pix -= 4;
		/* 8bpp and 16bpp */
		else
			pix -= bpp;
		--color;
		
		unsigned char *b = pix;
		unsigned char  c;
		
		/* 4bpp setup */
		if (is_4bit)
		{
			c = *b;
			b = &c;
			if (!(alt&1))
				c >>=  4;
			else
				c &=  15;
		}
		
		/* color-indexed */
		if (is_ci)
		{
			/* the * 2 is b/c ci textures have 16-bit color palettes */
			
			/* 4bpp */
			if (is_4bit)
				b = pal + c * 2;
			else
				b = pal + *pix * 2;
		}
		
		/* convert pixel */
		n64_colorfunc(color, b);
		
		++alt;
	}
}


static
inline
void
texture_to_n64(
	void n64_colorfunc(struct vec4b_2n64 *color, unsigned char *b)
	, unsigned char *dst
	, unsigned char *pix
	, unsigned char *pal
	, int pal_colors
	, int is_ci
	, enum n64texconv_bpp bpp
	, int w
	, int h
	, unsigned int *sz
)
{
	/* color points to last color */
	struct vec4b_2n64 *color = (struct vec4b_2n64*)(pix);
	int is_4bit = (bpp == N64TEXCONV_4);
	int alt = 0;
	int i;
	if (is_4bit && pal_colors > 16)
		pal_colors = 16;
	
	/* determine resulting size */
	*sz = get_size_bytes(w, h, 0/*FIXME*/, bpp);
	
	/* already in desirable format */
	if (bpp == N64TEXCONV_32)
		return;
	
	for (i=0; i < w * h; ++i, ++color)
	{
		unsigned char *b = dst;
		unsigned char  c;
		
		/* 4bpp setup */
		if (is_4bit)
		{
			c = *b;
			b = &c;
		}
		/* color-indexed */
		if (is_ci)
		{
			/* the * 2 is b/c ci textures have 16-bit color palettes */
			struct vec4b_2n64 Pcolor;
			struct vec4b_2n64 nearest = {255, 255, 255, 255};
			int nearest_idx = 0;
			int margin = 4; /* margin of error allowed */
			int Pidx;
			float nearestRGB = 1.0f;
			
			/* this is confirmed working on alpha pixels; try *
			 * importing eyes-xlu.png with palette 0x5C00 in  *
			 * object_link_boy.zobj                           */
			
			/* step through every color in the palette */
			for (Pidx = 0; Pidx < pal_colors; ++Pidx)
			{
				//if (Pidx >= 256)
				//	fprintf(stderr, "pidx = %d\n", Pidx);
				vec4b_2n64_from_rgba5551(&Pcolor, pal + Pidx * 2);
				
				/* measure difference between colors */
				struct vec4b_2n64 diff = {
					diff_int(Pcolor.x, color->x)
					, diff_int(Pcolor.y, color->y)
					, diff_int(Pcolor.z, color->z)
					, diff_int(Pcolor.w, color->w)
				};
				
				float diffR = (diff.x / 255.0f);
				float diffG = (diff.y / 255.0f);
				float diffB = (diff.z / 255.0f);
				float diffRGB = (diffR + diffG + diffB) / 3.0f;
				
				/* new nearest color */
				if (/*diff.x <= margin + nearest.x
					&& diff.y <= margin + nearest.y
					&& diff.z <= margin + nearest.z*/
					diffRGB <= nearestRGB
					&& diff.w <= margin + nearest.w
				)
				{
					nearest_idx = Pidx;
					nearest = diff;
					nearestRGB = diffRGB;
					
					/* exact match, safe to break */
					if (diff.x == 0
						&& diff.y == 0
						&& diff.z == 0
						&& diff.w == 0
					)
						break;
				}
			}
			
			/* 4bpp */
			if (is_4bit)
				c = nearest_idx;
			else
				*dst = nearest_idx;
		}
		
		/* convert pixel */
		else
			n64_colorfunc(color, b);
		
		/* 4bpp specific */
		if (is_4bit)
		{
			/* clear when we initially find it */
			if (!(alt&1))
				*dst = 0;
			
			/* how to transform result */
			if (!(alt&1))
				c <<=  4;
			else
				c &=  15;
			
			*dst |= c;
		}
		
		/* pixel advancement logic */
		/* 4bpp */
		if (is_4bit)
			dst += ((alt++)&1);
		/* 32bpp */
		else if (bpp == N64TEXCONV_32)
			dst += 4;
		/* 8bpp and 16bpp */
		else
			dst += bpp;
	}
}


const char *
n64texconv_to_rgba8888(
	unsigned char *dst
	, unsigned char *pix
	, unsigned char *pal
	, enum n64texconv_fmt fmt
	, enum n64texconv_bpp bpp
	, int w
	, int h
)
{
	/* error strings this function can return */
	static const char errstr_badformat[]  = "invalid format";
	static const char errstr_dimensions[] = "invalid dimensions (<= 0)";
	static const char errstr_palette[]    = "no palette";
	static const char errstr_texture[]    = "no texture";
	static const char errstr_dst[]        = "no destination buffer";

	/* no destination buffer */
	if (!dst)
		return errstr_dst;
	
	/* no texture */
	if (!pix)
		return errstr_texture;
	
	/* invalid format */
	if (
		fmt >= N64TEXCONV_FMT_MAX
		|| bpp > N64TEXCONV_32
		|| n64_colorfunc_array[fmt * 4 + bpp] == 0
	)
		return errstr_badformat;
	
	/* invalid dimensions (are <= 0) */
	if (w <= 0 || h <= 0)
		return errstr_dimensions;
	
	/* no palette to accompany ci texture data */
	if (fmt == N64TEXCONV_CI && pal == 0)
		return errstr_palette;
	
	/* convert texture using appropriate pixel converter */
	texture_to_rgba8888(
		n64_colorfunc_array[fmt * 4 + bpp]
		, dst
		, pix
		, pal
		, fmt == N64TEXCONV_CI
		, bpp
		, w
		, h
	);
	
	/* success */
	return 0;
}


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
)
{
	/* error strings this function can return */
	static const char errstr_badformat[]  = "invalid format";
	static const char errstr_dimensions[] = "invalid dimensions (<= 0)";
	static const char errstr_dst[]        = "no buffer";
	static const char errstr_palette[]    = "no palette";
	
	unsigned int sz_unused;

	/* no src/dst buffers defined */
	if (!dst || !pix)
		return errstr_dst;
	
	/* sz */
	if (!sz)
		sz = &sz_unused;
	
	/* invalid format */
	if (
		fmt >= N64TEXCONV_FMT_MAX
		|| bpp > N64TEXCONV_32
		|| n64_colorfunc_array[fmt * 4 + bpp] == 0
	)
		return errstr_badformat;
	
	/* ci format requires pal and pal_colors */
	if (fmt == N64TEXCONV_CI && (!pal || pal_colors <= 0))
		return "ci format but no palette provided";
	
	/* invalid dimensions (are <= 0) */
	if (w <= 0 || h <= 0)
		return errstr_dimensions;
	
	/* no palette to accompany ci texture data */
	if (fmt == N64TEXCONV_CI && pal == 0)
		return errstr_palette;
	
	/* convert texture using appropriate pixel converter */
	texture_to_n64(
		n64_colorfunc_array_to[fmt * 4 + bpp]
		, dst
		, pix
		, pal
		, pal_colors
		, fmt == N64TEXCONV_CI
		, bpp
		, w
		, h
		, sz
	);
	
	/* success */
	return 0;
}

