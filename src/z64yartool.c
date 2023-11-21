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

#include "common.h"
#include "yar.h" // from z64compress

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
		fprintf(stdout, "32x32,rgba32,%08x.png\n", this->dataAddrUnyar);
	
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
	return EXIT_SUCCESS;
}

static int YarBuild(const char *input)
{
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
	fprintf(stderr, "build date: TODO\n");
	
	if (strcmp(command, "unyar") && argc != 3)
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
 
