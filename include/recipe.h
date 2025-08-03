/*
 * recipe.h
 *
 * for loading recipe files
 *
 */

#ifndef RECIPE_H_INCLUDED
#define RECIPE_H_INCLUDED

#include <stdbool.h>

#include "n64texconv.h" // for texture formats

struct RecipeItem
{
	struct RecipeItem *next;
	char *imageFilename;
	int width;
	int height;
	int palId;
	int palMaxColors;
	enum n64texconv_fmt fmt;
	enum n64texconv_bpp bpp;
	unsigned int writeAt;
	unsigned int endOffset;
	void *data;
	bool isAlreadyWritten;
};

struct Recipe
{
	char *behavior;
	char *filename;
	char *directory;
	char *yarName;
	char *imageDir;
	struct RecipeItem *head;
	int count;
};

struct Recipe *RecipeRead(const char *filename);
void RecipeFree(struct Recipe *recipe);
void RecipePrint(struct Recipe *recipe);

#endif

