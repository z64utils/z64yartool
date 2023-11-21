/*
 * recipe.h
 *
 * for loading recipe files
 *
 */

#ifndef RECIPE_H_INCLUDED
#define RECIPE_H_INCLUDED

#include "n64texconv.h" // for texture formats

struct RecipeItem
{
	struct RecipeItem *next;
	char *imageFilename;
	int width;
	int height;
	enum n64texconv_fmt fmt;
	enum n64texconv_bpp bpp;
};

struct Recipe
{
	char *filename;
	char *directory;
	char *yarName;
	char *imageDir;
	struct RecipeItem *head;
};

struct Recipe *RecipeRead(const char *filename);
void RecipeFree(struct Recipe *recipe);
void RecipePrint(struct Recipe *recipe);

#endif

