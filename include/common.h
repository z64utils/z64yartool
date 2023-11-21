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

uint32_t U32read(const void *src);

#endif

