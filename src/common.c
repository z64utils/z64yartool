/*
 * common.c
 *
 * common helpers
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

/* minimal file loader
 * returns 0 on failure
 * returns pointer to loaded file on success
 */
void *FileLoad(const char *fn, size_t *sz)
{
   FILE *fp;
   void *dat;
   
   /* rudimentary error checking returns 0 on any error */
   if (
      !fn
      || !sz
      || !(fp = fopen(fn, "rb"))
      || fseek(fp, 0, SEEK_END)
      || !(*sz = ftell(fp))
      || fseek(fp, 0, SEEK_SET)
      || !(dat = malloc(*sz + 1))
      || fread(dat, 1, *sz, fp) != *sz
      || fclose(fp)
   )
      return 0;
   
   return dat;
}

char *FileLoadAsString(const char *fn)
{
	size_t sz;
	char *dst;
	
	if (!(dst = FileLoad(fn, &sz)))
		return 0;
	
	dst[sz] = '\0';
	
	return dst;
}

bool FileIsLoaded(const char *fn, const void *data)
{
	if (!data)
	{
		fprintf(stderr, "failed to load file '%s'\n", fn);
		return false;
	}
	
	return true;
}

uint32_t U32read(const void *src)
{
	const uint8_t *b = src;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3]);
}


