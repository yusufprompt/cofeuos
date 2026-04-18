#ifndef _TERMINUS_FONT_H
#define _TERMINUS_FONT_H

#include "types.h"

typedef struct {
    u32 magic;
    u32 version;
    u32 headersize;
    u32 flags;
    u32 length;
    u32 charsize;
    u32 height;
    u32 width;
} psf2_t;

extern unsigned char font_psf[];
extern unsigned int font_psf_len;

#endif /* _TERMINUS_FONT_H */
