

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H

#include "color.h"

#include <math.h>

typedef enum
{
    WRAP_CLAMP,
    WRAP_REPEAT,
    WRAP_MIRROR
} wrap_mode_t;
typedef enum
{
    FILTER_NEAREST,
    FILTER_BILINEAR
} filter_mode_t;

typedef struct texture_t texture_t;

texture_t *texture_create(const char *path, wrap_mode_t wrap, filter_mode_t filter,
                          bool generate_mipmaps);
void texture_free(texture_t *tex);

color_t texture_sample(const texture_t *tex, float u, float v, float lod);

#endif /* RASTERIZER_TEXTURE_H */