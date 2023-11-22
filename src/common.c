/*
 * common.c
 *
 * common helpers
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

void FilePutBE32(FILE *file, uint32_t value)
{
	fputc(value >> 24, file);
	fputc(value >> 16, file);
	fputc(value >>  8, file);
	fputc(value >>  0, file);
}

char *FileGetDirectory(const char *fn)
{
	char *out = Strdup(fn);
	char *ss0 = strrchr(out, '/');
	char *ss1 = strrchr(out, '\\'); // win32
	
	if (ss0 == ss1) // doesn't contain slashes
		strcpy(out, "");
	else if (ss0 > ss1)
		ss0[1] = '\0';
	else if (ss1 > ss0)
		ss1[1] = '\0';
	
	return out;
}

uint32_t U32read(const void *src)
{
	const uint8_t *b = src;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3]);
}

char *Strdup(const char *str)
{
	return strcpy(malloc(strlen(str) + 1), str);
}

char *StrdupContiguous(const char *str)
{
	int len = strcspn(str, " \r\n\t");
	char *dst = malloc(len + 1);
	
	assert(dst);
	
	strncpy(dst, str, len);
	
	dst[len] = '\0';
	
	return dst;
}

char *StringPrependInplace(char **srcdst, const char *prefix)
{
	char *tmp = malloc(strlen(prefix) + strlen(*srcdst) + 1);
	sprintf(tmp, "%s%s", prefix, *srcdst);
	free(*srcdst);
	*srcdst = tmp;
	return tmp;
}

const char *StringNextLine(const char *str)
{
	if (!str)
		return 0;
	
	while (*str && !strchr("\r\n\t", *str))
		++str;
	
	while (*str && strchr("\r\n\t", *str))
		++str;
	
	if (!*str)
		return 0;
	
	return str;
}


