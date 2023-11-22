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

static int YarDump(const char *input)
{
	struct Recipe *recipe = RecipeRead(input);
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
	
	RecipeFree(recipe);
	YarFree(yar);
	
	free(buffer);
	free(bufferOut);
	return EXIT_SUCCESS;
}

static int YarBuild(const char *input)
{
	struct Recipe *recipe = RecipeRead(input);
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
		free(pix);
	}
	
	// alignment
	while (ftell(out) & 15)
		fputc(0, out);
	
	// header
	fseek(out, 4, SEEK_SET);
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
		FilePutBE32(out, this->endOffset);
	
	fclose(out);
	RecipeFree(recipe);
	yazCtx_free(yazCtx);
	free(buffer);
	return EXIT_SUCCESS;
}

static void ShowArgs(void)
{
	#define OUT(X) fprintf(stderr, X "\n");
	
	OUT("usage examples:")
	OUT(" z64yartool stat input.yar > recipe.txt")
	OUT(" z64yartool unyar input.yar output.bin")
	OUT(" z64yartool dump recipe.txt")
	OUT(" z64yartool build recipe.txt")
	
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
	
	fprintf(stderr, "welcome to z64yartool v1.0.0 <z64.me>\n");
	fprintf(stderr, "build date: %s at %s\n", __DATE__, __TIME__);
	
	if ((!command || strcmp(command, "unyar")) && argc != 3)
		ShowArgsAndExit();
	
	if (!strcmp(command, "stat"))
		return YarStat(input);
	else if (!strcmp(command, "dump"))
		return YarDump(input);
	else if (!strcmp(command, "build"))
		return YarBuild(input);
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
 
