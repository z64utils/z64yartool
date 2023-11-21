/* yaz.h -- imported from z64compress, with tweaks */

#ifndef YAZ_H_INCLUDED
#define YAZ_H_INCLUDED

int yazenc(void *src, unsigned src_sz, void *dst, unsigned *dst_sz, void *_ctx);
void *yazCtx_new(void);
void yazCtx_free(void *_ctx);
int yazdec(void *_src, void *_dst, unsigned dstSz, unsigned *srcSz);

#endif /* YAZ_H_INCLUDED */

