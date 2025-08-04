/*
 * z64yartool <z64.me>
 *
 * stats, unyars, dumps, and build Majora's Mask .yar files
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/stat.h> // directory creation
#include <sys/types.h>

#include "common.h"
#include "yar.h" // from z64compress
#include "yaz.h" // from z64compress
#include "n64texconv.h" // from z64convert
#include "recipe.h"
#include "stb_image_write.h"
#include "stb_image.h"
#include "exoquant.h"

struct YarEntry
{
	struct YarEntry *next;
	void *data;
	unsigned int dataAddrUnyar; // where data will live in unyar'd file
};

struct Yar
{
	struct YarEntry *head;
	int count;
	void *data;
	size_t dataSz;
};

struct Yar *YarRead(const char *filename)
{
	struct Yar *yar = calloc(1, sizeof(*yar));
	struct YarEntry *prev = 0;
	uint8_t *body;
	uint8_t *stepHeader;
	unsigned int dataAddrUnyar = 0;
	
	assert(yar);
	
	if (!(yar->data = FileLoad(filename, &yar->dataSz)))
		return 0;
		
	stepHeader = yar->data;
	body = stepHeader + U32read(yar->data);
	
	yar->count = (U32read(yar->data) / sizeof(uint32_t)) - 1;
	
	for (int i = 0; i < yar->count; ++i)
	{
		struct YarEntry *this = calloc(1, sizeof(*this));
		
		assert(this);
		
		if (prev)
			prev->next = this;
		else
			yar->head = this;
		prev = this;
		
		this->data = body + U32read(stepHeader);
		if (stepHeader == yar->data) // first file
			this->data = body;
		this->dataAddrUnyar = dataAddrUnyar;
		dataAddrUnyar += U32read(((uint8_t*)this->data) + 4); // decompressed size
		stepHeader += sizeof(uint32_t);
	}
	
	return yar;
}

void YarFree(struct Yar *yar)
{
	struct YarEntry *next = 0;
	
	for (struct YarEntry *this = yar->head; this; this = next)
	{
		next = this->next;
		
		free(this);
	}
	
	if (yar->data)
		free(yar->data);
	
	free(yar);
}

static int YarStat(const char *input)
{
	struct Yar *yar = YarRead(input);
	const char *period = strrchr(input, '.');
	
	assert(period);
	
	if (!yar)
		return EXIT_FAILURE;
	
	fprintf(stdout, "%s # relative path\n", input);
	fprintf(stdout, "%.*s/ # where the images live\n", (int)(period - input), input);
	
	for (struct YarEntry *this = yar->head; this; this = this->next)
		fprintf(stdout, "??x??,unknown,%08x.png\n", this->dataAddrUnyar);
	
	YarFree(yar);
	return EXIT_SUCCESS;
}

static int YarUnyar(const char *infn, const char *outfn)
{
	if (unyar(infn, outfn, true))
	{
		fprintf(stderr, "unyar '%s' into '%s' failed\n", infn, outfn);
		
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

static int YarDump(struct Recipe *recipe)
{
	struct Yar *yar = YarRead(recipe->yarName);
	struct YarEntry *yarEntry;
	void *buffer = malloc(512 * 1024); // 512 KiB is plenty
	void *bufferOut = malloc(1024 * 1024);
	
	assert(buffer);
	
	if (!yar)
		return EXIT_FAILURE;
	
	RecipePrint(recipe);
	
#ifdef _WIN32
	mkdir(recipe->imageDir);
#else
	mkdir(recipe->imageDir, 0777);
#endif
	yarEntry = yar->head;
	for (struct RecipeItem *this = recipe->head
		; this && yarEntry
		; this = this->next, yarEntry = yarEntry->next
	)
	{
		unsigned unused;
		
		// decompress the compressed texture
		spinout_yaz_dec(yarEntry->data, buffer, 0, &unused);
		
		// convert to standard 32-bit rgba
		// TODO fix n64texconv_to_rgba8888 so in-place 4-bit conversions don't corrupt first pixel
		//      (using two buffers is an acceptable solution in the meantime)
		n64texconv_to_rgba8888(
			bufferOut
			, buffer
			, 0
			, this->fmt
			, this->bpp
			, this->width
			, this->height
		);
		
		// write as png
		fprintf(stderr, "writing '%s'\n", this->imageFilename);
		stbi_write_png(this->imageFilename, this->width, this->height, 4, bufferOut, this->width * 4);
	}
	
	YarFree(yar);
	
	free(buffer);
	free(bufferOut);
	return EXIT_SUCCESS;
}

static int YarBuild(struct Recipe *recipe)
{
	void *buffer = malloc(512 * 1024); // 512 KiB is plenty
	void *yazCtx = yazCtx_new();
	FILE *out;
	unsigned int rel;
	
	assert(buffer);
	assert(recipe);
	
	// TODO zzrtl doesn't like the filesize changing, so use rb+ instead of wb for now
	//if (!(out = fopen(recipe->yarName, "wb")))
	if (!(out = fopen(recipe->yarName, "rb+")))
	{
		fprintf(stderr, "failed to open '%s' for writing\n", recipe->yarName);
		exit(EXIT_FAILURE);
	}
	
	// header
	FilePutBE32(out, (recipe->count + 1) * sizeof(uint32_t));
	for (int i = 0; i < recipe->count; ++i)
		FilePutBE32(out, 0);
	rel = ftell(out);
	
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
	{
		const char *imgFn = this->imageFilename;
		const char *errmsg = 0;
		void *pix;
		int w = 0;
		int h = 0;
		int unused;
		unsigned int sz;
		
		// load image
		if (!(pix = stbi_load(imgFn, &w, &h, &unused, STBI_rgb_alpha)))
		{
			fprintf(stderr, "failed to load image '%s'\n", imgFn);
			exit(EXIT_FAILURE);
		}
		
		// assert no size change
		if (this->width != w || this->height != h)
		{
			fprintf(stderr, "'%s' image unexpected dimensions\n", imgFn);
			exit(EXIT_FAILURE);
		}
		
		// convert to n64 pixel format
		if ((errmsg = n64texconv_to_n64(pix, pix, 0, -1, this->fmt, this->bpp, w, h, &sz)))
		{
			fprintf(stderr, "'%s' conversion error: %s\n", imgFn, errmsg);
			exit(EXIT_FAILURE);
		}
		
		// compress
		if (yazenc(pix, sz, buffer, &sz, yazCtx))
		{
			fprintf(stderr, "compression error\n");
			exit(EXIT_FAILURE);
		}
		
		// write
		if (fwrite(buffer, 1, sz, out) != sz)
		{
			fprintf(stderr, "error writing to file '%s'\n", recipe->yarName);
			exit(EXIT_FAILURE);
		}
		
		// alignment
		while (ftell(out) & 3)
			fputc(0, out);
		
		// used for header later
		this->endOffset = ftell(out) - rel;
		
		// cleanup
		stbi_image_free(pix);
	}
	
	// alignment
	while (ftell(out) & 15)
		fputc(0, out);
	
	// header
	fseek(out, 4, SEEK_SET);
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
		FilePutBE32(out, this->endOffset);
	
	fclose(out);
	yazCtx_free(yazCtx);
	free(buffer);
	return EXIT_SUCCESS;
}

static int RetextureDump(struct Recipe *recipe)
{
	size_t dataSz;
	uint8_t *data = FileLoad(recipe->yarName, &dataSz);
	void *buffer = malloc(512 * 1024); // 512 KiB is plenty
	int rval = EXIT_SUCCESS;
	
	assert(buffer);
	
	if (!data)
	{
		fprintf(stderr, "failed to read file '%s'\n", recipe->yarName);
		return EXIT_FAILURE;
	}
	
#ifdef _WIN32
	mkdir(recipe->imageDir);
#else
	mkdir(recipe->imageDir, 0777);
#endif
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
	{
		uint8_t *palette = 0;
		const char *imageFn = this->imageFilename;
		
		// is palette
		if (this->palMaxColors)
		{
			// is generated automatically
			if (!strcmp(imageFn, "auto"))
				continue;
			
			this->width = this->palMaxColors;
			this->height = 1;
		}
		
		if (this->fmt == N64TEXCONV_CI)
		{
			for (struct RecipeItem *i = recipe->head; i; i = i->next)
				if (i->palMaxColors && i->palId == this->palId)
					palette = data + i->writeAt;
			
			if (!palette)
			{
				fprintf(stderr, "failed to find palette %d for texture %s\n"
					, this->palId, imageFn
				);
				rval = EXIT_FAILURE;
				break;
			}
		}
		
		// convert to standard 32-bit rgba
		n64texconv_to_rgba8888(
			buffer
			, data + this->writeAt
			, palette
			, this->fmt
			, this->bpp
			, this->width
			, this->height
		);
		
		// write as png
		fprintf(stderr, "writing '%s'\n", imageFn);
		stbi_write_png(imageFn, this->width, this->height, 4, buffer, this->width * 4);
	}
	
	free(data);
	free(buffer);
	return rval;
}

static int RetextureBuildInject(struct RecipeItem *this, uint8_t **data, size_t *dataSz)
{
	const char *imgFn = this->imageFilename;
	const char *errmsg = 0;
	void *pix;
	int w = 0;
	int h = 0;
	int unused;
	unsigned int sz;
	
	// skip those already written
	if (this->isAlreadyWritten)
		return EXIT_SUCCESS;
	
	this->isAlreadyWritten = true;
	
	// color-indexed textures are handled in a previous step
	if (this->fmt == N64TEXCONV_CI)
	{
		// so this should never be printed; if this texture was
		// handled in a previous step, isAlreadyWritten == true
		fprintf(stderr, "skipping '%s'\n", imgFn);
		return EXIT_SUCCESS;
	}
	
	// load image
	if (!(pix = stbi_load(imgFn, &w, &h, &unused, STBI_rgb_alpha)))
	{
		fprintf(stderr, "failed to load image '%s'\n", imgFn);
		exit(EXIT_FAILURE);
	}
	
	// assert no size change
	if (this->width != w || this->height != h)
	{
		fprintf(stderr, "'%s' image unexpected dimensions\n", imgFn);
		exit(EXIT_FAILURE);
	}
	
	// convert to n64 pixel format
	if ((errmsg = n64texconv_to_n64(pix, pix, 0, -1, this->fmt, this->bpp, w, h, &sz)))
	{
		fprintf(stderr, "'%s' conversion error: %s\n", imgFn, errmsg);
		exit(EXIT_FAILURE);
	}
	
	// write
	if (this->writeAt == (unsigned int)-1)
	{
		fprintf(stderr, "no offset specified for image '%s'\n", imgFn);
		exit(EXIT_FAILURE);
	}
	if (this->writeAt + sz > *dataSz)
	{
		*dataSz = (this->writeAt + sz) * 2;
		*data = realloc(*data, *dataSz);
	}
	memcpy((*data) + this->writeAt, pix, sz);
	
	// cleanup
	stbi_image_free(pix);
	return EXIT_SUCCESS;
}

static int RetextureBuild(struct Recipe *recipe)
{
	size_t dataSz;
	uint8_t *data = FileLoad(recipe->yarName, &dataSz);
	uint8_t *buffer = malloc(512 * 1024); // 512 KiB is plenty
	uint8_t *buffer2 = malloc(512 * 1024);
	uint8_t *writeHead = buffer;
	uint8_t *palette;
	exq_data *quant;
	int rval = EXIT_SUCCESS;
	unsigned int sz;
	
	assert(buffer);
	
	if (!data)
	{
		fprintf(stderr, "failed to read file '%s'\n", recipe->yarName);
		return EXIT_FAILURE;
	}
	
#ifdef _WIN32
	mkdir(recipe->imageDir);
#else
	mkdir(recipe->imageDir, 0777);
#endif
	
	// first pass: construct the palettes
	for (struct RecipeItem *pal = recipe->head; pal; pal = pal->next)
	{
		// is not palette
		if (pal->palMaxColors == 0)
			continue;
		
		// load all the images into buffer
		for (struct RecipeItem *this = recipe->head; this; this = this->next)
		{
			const char *imgFn = this->imageFilename;
			const char *errmsg;
			void *pix;
			int w;
			int h;
			int unused;
			
			// skip images that aren't using this palette
			if (this->fmt != N64TEXCONV_CI || pal->palId != this->palId)
				continue;
			
			// load image
			if (!(pix = stbi_load(imgFn, &w, &h, &unused, STBI_rgb_alpha)))
			{
				fprintf(stderr, "failed to load image '%s'\n", imgFn);
				exit(EXIT_FAILURE);
			}
			
			// assert no size change
			if (this->width != w || this->height != h)
			{
				fprintf(stderr, "'%s' image unexpected dimensions\n", imgFn);
				exit(EXIT_FAILURE);
			}
			
			// simplify colors to make for easier quantization
			if ((errmsg = n64texconv_to_n64_and_back(pix, 0, 0, N64TEXCONV_RGBA, N64TEXCONV_16, w, h)))
			{
				fprintf(stderr, "'%s' conversion error: %s\n", imgFn, errmsg);
				exit(EXIT_FAILURE);
			}
			
			// write into buffer
			this->udata = writeHead;
			memcpy(writeHead, pix, w * h * STBI_rgb_alpha);
			writeHead += w * h * STBI_rgb_alpha;
			
			stbi_image_free(pix);
		}
		
		// quantize them and construct palette
		{
			palette = writeHead;
			quant = exq_init();
			
			// if palette name != auto, load it from image file
			if (strcmp(pal->imageFilename, "auto"))
			{
				void *pix;
				const char *imgFn = pal->imageFilename;
				int w;
				int h;
				int unused;
				
				// load image
				if (!(pix = stbi_load(imgFn, &w, &h, &unused, STBI_rgb_alpha)))
				{
					fprintf(stderr, "failed to load palette '%s'\n", imgFn);
					exit(EXIT_FAILURE);
				}
				
				// assert size hasn't changed
				if (w * h < pal->palMaxColors)
				{
					fprintf(stderr, "'%s' palette contains too few pixels\n", imgFn);
					exit(EXIT_FAILURE);
				}
				else if (w * h > pal->palMaxColors)
				{
					fprintf(stderr, "warning: '%s' contains %d pixels, but only "
						"the first %d will be used\n", imgFn, w * h, pal->palMaxColors
					);
				}
				
				// upload original palette
				exq_feed(quant, pix, pal->palMaxColors);
				stbi_image_free(pix);
			}
			// otherwise, generate one from the textures
			else
				exq_feed(quant, buffer, (writeHead - buffer) / STBI_rgb_alpha);
			
			// finalize palette
			exq_quantize_hq(quant, pal->palMaxColors);
			exq_get_palette(quant, palette, pal->palMaxColors);
		}
		
		// apply palette to images as they are written
		for (struct RecipeItem *this = recipe->head; this; this = this->next)
		{
			void *udata = this->udata;
			uint8_t *dst = data + this->writeAt;
			int w = this->width;
			int h = this->height;
			int bytes = w * h;
			
			// skip images that aren't using this palette
			if (!udata)
				continue;
			
			if (strstr(recipe->behavior, "random-dither"))
				exq_map_image_dither(quant, w, h, this->udata, buffer2, 0);
			else if (strstr(recipe->behavior, "dither"))
				exq_map_image_ordered(quant, w, h, this->udata, buffer2);
			else
				exq_map_image(quant, w * h, this->udata, buffer2);
			
			// 4-bit
			if (this->bpp == N64TEXCONV_4)
			{
				// squash 8-bit bytes '01 02 03 04' into 4-bit '12 34'
				bytes /= 2;
				for (int i = 0; i < bytes; ++i)
					buffer2[i] = (buffer2[i * 2] << 4) | (buffer2[i * 2 + 1] & 0xf);
			}
			
			// write the texture
			memcpy(dst, buffer2, bytes);
			this->udata = 0;
			this->isAlreadyWritten = true;
		}
		
		// write the palette
		n64texconv_to_n64(data + pal->writeAt, palette, 0, -1, pal->fmt, pal->bpp, pal->palMaxColors, 1, &sz);
		pal->isAlreadyWritten = true;
		exq_free(quant);
	}
	
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
	{
		RetextureBuildInject(this, &data, &dataSz);
	}
	
	FILE *out = fopen(recipe->yarName, "wb");
	if (fwrite(data, 1, dataSz, out) != dataSz)
	{
		fprintf(stderr, "error writing to file '%s'\n", recipe->yarName);
		exit(EXIT_FAILURE);
	}
	fclose(out);
	
	free(data);
	free(buffer);
	free(buffer2);
	return rval;
}

static void ShowArgs(void)
{
	#define OUT(X) fprintf(stderr, X "\n");
	
	OUT("usage examples:")
	OUT(" z64yartool stat input.yar > recipe.txt")
	OUT(" z64yartool unyar input.yar output.bin")
	OUT(" z64yartool dump recipe.txt")
	OUT(" z64yartool build recipe.txt")
	OUT(" z64yartool print recipe.txt")
	
	#undef OUT
}

static void ShowArgsAndExit(void)
{
	ShowArgs();
	exit(EXIT_FAILURE);
}

int main(int argc, const char *argv[])
{
	const char *command = argv[1];
	const char *input = argv[2];
	
	fprintf(stderr, "welcome to z64yartool v1.1.0 <z64.me> special thanks Javarooster\n");
	fprintf(stderr, "build date: %s at %s\n", __DATE__, __TIME__);
	
	if ((!command || strcmp(command, "unyar")) && argc != 3)
		ShowArgsAndExit();
	
	if (!strcmp(command, "stat"))
		return YarStat(input);
	else if (!strcmp(command, "dump")
		|| !strcmp(command, "build")
		|| !strcmp(command, "print")
	)
	{
		struct Recipe *recipe = RecipeRead(input);
		bool isDump = !strcmp(command, "dump");
		int rval = 0;
		
		if (!strcmp(command, "print"))
		{
			RecipePrint(recipe);
		}
		else if (recipe->behavior[0] == '*')
		{
			if (isDump)
				rval = RetextureDump(recipe);
			else
				rval = RetextureBuild(recipe);
		}
		else
		{
			if (isDump)
				rval = YarDump(recipe);
			else
				rval = YarBuild(recipe);
		}
		
		RecipeFree(recipe);
		return rval;
	}
	else if (!strcmp(command, "unyar"))
	{
		const char *output = argv[3];
		
		if (argc != 4)
			ShowArgsAndExit();
		
		return YarUnyar(input, output);
	}
	else
	{
		fprintf(stderr, "unknown command '%s'\n", command);
		ShowArgsAndExit();
	}
	
	return EXIT_SUCCESS;
}
 
