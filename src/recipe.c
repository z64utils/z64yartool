/*
 * recipe.c
 *
 * for loading recipe files
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "recipe.h"
#include "common.h"

struct Recipe *RecipeRead(const char *filename)
{
	struct Recipe *recipe = calloc(1, sizeof(*recipe));
	char *data = FileLoadAsString(filename);
	struct RecipeItem *prev = 0;
	const char *step;
	 // color-indexed formats are unsupported for now
	const char *knownFmt = "rgba16, rgba32, ia4, ia8, ia16, i4, i8";
	const char *behaviors = "z64retexture, etc";
	
	if (!data)
	{
		fprintf(stderr, "failed to load file '%s'\n", filename);
		exit(EXIT_FAILURE);
	}
	
	assert(recipe);
	
	step = data;
	while (*step == '#')
		step = StringNextLine(step);
	
	recipe->filename = Strdup(filename);
	recipe->directory = FileGetDirectory(filename);
	recipe->behavior = StrdupContiguous(step);
	
	step = StringNextLine(step);
	assert(step);
	
	// not a behavior
	if (!strstr(behaviors, recipe->behavior))
	{
		recipe->yarName = Strdup(recipe->behavior);
		strcpy(recipe->behavior, "");
	}
	else
	{
		recipe->yarName = StrdupContiguous(step);
	}
	
	step = StringNextLine(step);
	recipe->imageDir = StrdupContiguous(step);
	
	// ensure ends in a slash
	if ((strrchr(recipe->imageDir, '/') + 1) != (recipe->imageDir + strlen(recipe->imageDir)))
	{
		fprintf(stderr, "imageDir '%s' does not end in '/' as expected, please add one\n", recipe->imageDir);
		exit(EXIT_FAILURE);
	}
	
	// guarantee relative paths
	StringPrependInplace(&recipe->yarName, recipe->directory);
	StringPrependInplace(&recipe->imageDir, recipe->directory);
	
	for (step = StringNextLine(step); step; step = StringNextLine(step))
	{
		struct RecipeItem *this = calloc(1, sizeof(*this));
		char *tmp = StrdupContiguous(step);
		char fmt[32];
		char filename[512];
		int palId = -1;
		int palMaxColors = 0;
		int width;
		int height;
		unsigned int writeAt = -1;
		
		recipe->count += 1;
		
		assert(this);
		
		if (sscanf(tmp, "%dx%d,%[^,],%[^, #\r\n],%x", &width, &height, fmt, filename, &writeAt) < 4
			&& sscanf(tmp, "%d,pal-%d,%[^,],%[^, #\r\n],%x", &palMaxColors, &palId, fmt, filename, &writeAt) < 4
		)
		{
			fprintf(stderr, "unexpectedly formatted line: '%s'\n", tmp);
			exit(EXIT_FAILURE);
		}
		
		// TODO check each combination individually since there are so few
		if (strstr(fmt, "rgba"))
			this->fmt = N64TEXCONV_RGBA;
		else if (strstr(fmt, "ci")) // color-indexed formats are unsupported for now
			this->fmt = N64TEXCONV_CI;
		else if (strstr(fmt, "ia"))
			this->fmt = N64TEXCONV_IA;
		else if (strstr(fmt, "i"))
			this->fmt = N64TEXCONV_I;
		else
		{
			fprintf(stderr, "unknown texture format '%s', valid formats:\n%s\n", fmt, knownFmt);
			exit(EXIT_FAILURE);
		}
		if (strstr(fmt, "4"))
			this->bpp = N64TEXCONV_4;
		else if (strstr(fmt, "8"))
			this->bpp = N64TEXCONV_8;
		else if (strstr(fmt, "16"))
			this->bpp = N64TEXCONV_16;
		else if (strstr(fmt, "32"))
			this->bpp = N64TEXCONV_32;
		else
		{
			fprintf(stderr, "unknown texture format '%s', valid formats:\n%s\n", fmt, knownFmt);
			exit(EXIT_FAILURE);
		}
		
		// get palette id
		if (this->fmt == N64TEXCONV_CI)
		{
			if (!strrchr(fmt, '-')
				|| sscanf(strrchr(fmt, '-') + 1, "%d", &palId) != 1
			)
			{
				fprintf(stderr, "provided ci format w/o specifying which palette: '%s'\n", fmt);
				fprintf(stderr, "(ci4 and ci8 are expected to end in -n, where n = palette id)\n");
				fprintf(stderr, "(for example: ci8-0 specifies ci8 format using palette 0)\n");
				exit(EXIT_FAILURE);
			}
		}
		
		this->width = width;
		this->height = height;
		this->writeAt = writeAt;
		this->palId = palId;
		this->palMaxColors = palMaxColors;
		
		if (palMaxColors && !strcmp(filename, "auto"))
			this->imageFilename = Strdup(filename);
		else
		{
			this->imageFilename = malloc(strlen(recipe->imageDir) + strlen(filename) + 1);
			sprintf(this->imageFilename, "%s%s", recipe->imageDir, filename);
		}
		
		if (prev)
			prev->next = this;
		else
			recipe->head = this;
		prev = this;
		
		free(tmp);
	}
	
	free(data);
	return recipe;
}

void RecipeFree(struct Recipe *recipe)
{
	struct RecipeItem *next = 0;
	
	for (struct RecipeItem *this = recipe->head; this; this = next)
	{
		free(this->imageFilename);
		next = this->next;
		free(this);
	}
	
	free(recipe->filename);
	free(recipe->directory);
	free(recipe->yarName);
	free(recipe->imageDir);
	free(recipe->behavior);
	free(recipe);
}

void RecipePrint(struct Recipe *recipe)
{
	fprintf(stderr, "recipe info:\n");
	fprintf(stderr, " filename: '%s'\n", recipe->filename);
	fprintf(stderr, " directory: '%s'\n", recipe->directory);
	fprintf(stderr, " yarName: '%s'\n", recipe->yarName);
	fprintf(stderr, " imageDir: '%s'\n", recipe->imageDir);
	fprintf(stderr, " items:\n");
	
	for (struct RecipeItem *this = recipe->head; this; this = this->next)
	{
		fprintf(stderr, "  '%s', %dx%d", this->imageFilename, this->width, this->height);
		if (this->writeAt != (unsigned int)-1) fprintf(stderr, ", .writeAt = 0x%x", this->writeAt);
		if (this->palId >= 0) fprintf(stderr, ", .palId = %d", this->palId);
		fprintf(stderr, "\n");
	}
}

