/*
 * recipe.h
 *
 * for loading recipe files
 *
 */

#ifndef RECIPE_H_INCLUDED
#define RECIPE_H_INCLUDED

struct RecipeItem
{
	struct RecipeItem *next;
	char *imageFilename;
	int width;
	int height;
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

