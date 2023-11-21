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
	
	if (!data)
	{
		fprintf(stderr, "failed to load file '%s'\n", filename);
		exit(EXIT_FAILURE);
	}
	
	assert(recipe);
	
	recipe->filename = Strdup(filename);
	recipe->directory = FileGetDirectory(filename);
	recipe->yarName = StrdupContiguous(data);
	
	step = data;
	step = StringNextLine(step);
	assert(step);
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
		int width;
		int height;
		
		assert(this);
		
		if (sscanf(tmp, "%dx%d,%[^,]", &width, &height, fmt) != 3)
		{
			fprintf(stderr, "unexpectedly formatted line: '%s'\n", tmp);
			exit(EXIT_FAILURE);
		}
		
		this->width = width;
		this->height = height;
		this->imageFilename = malloc(strlen(recipe->imageDir) + strlen(tmp) + 1);
		sprintf(this->imageFilename, "%s%s", recipe->imageDir, strrchr(tmp, ',') + 1);
		
		if (prev)
			prev->next = this;
		else
			recipe->head = this;
		prev = this;
		
		free(tmp);
	}
	
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
		fprintf(stderr, "  '%s', %dx%d\n", this->imageFilename, this->width, this->height);
}
