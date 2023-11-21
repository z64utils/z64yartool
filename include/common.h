/*
 * common.h
 *
 * common helpers
 *
 */

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

void *FileLoad(const char *fn, size_t *sz);
char *FileLoadAsString(const char *fn);
bool FileIsLoaded(const char *fn, const void *data);
char *FileGetDirectory(const char *fn);

uint32_t U32read(const void *src);

char *Strdup(const char *str);
char *StrdupContiguous(const char *str);
const char *StringNextLine(const char *str);
char *StringPrependInplace(char **srcdst, const char *prefix);

#endif

