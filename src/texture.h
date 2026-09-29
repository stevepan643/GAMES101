#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H

#include <math.h>
#include "color.h"

typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} texture_t;

typedef enum { WRAP_CLAMP, WRAP_REPEAT, WRAP_MIRROR } wrap_mode_t;
typedef enum { FILTER_NEAREST, FILTER_BILINEAR } filter_mode_t;

static inline float wrap_coord(float x, wrap_mode_t wrap)
{
    switch (wrap) {
    case WRAP_REPEAT:
        return x - floorf(x);

    case WRAP_MIRROR: {
        x = fabsf(x);
        x = fmodf(x, 2.0f);
        return x > 1.0f ? 2.0f - x : x;
    }

    case WRAP_CLAMP:
    default:
        if (x < 0.0f) return 0.0f;
        if (x > 1.0f) return 1.0f;
        return x;
    }
}

static inline color_t get_pixel(const texture_t *tex, int x, int y)
{
    int idx = (y * tex->width + x) * tex->channels;
    color_t c;
    c.r = tex->data[idx + 0];
    c.g = tex->data[idx + 1];
    c.b = tex->data[idx + 2];
    c.a = tex->data[idx + 3];
    return c;
}

static inline color_t lerp_color(color_t a, color_t b, float t)
{
    color_t c;
    c.r = (unsigned char)(a.r + (b.r - a.r) * t);
    c.g = (unsigned char)(a.g + (b.g - a.g) * t);
    c.b = (unsigned char)(a.b + (b.b - a.b) * t);
    c.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return c;
}

static inline color_t texture_sample(const texture_t *tex, float u, float v,
                                     wrap_mode_t wrap, filter_mode_t filter)
{
    u = wrap_coord(u, wrap);
    v = wrap_coord(v, wrap);

    float fx = u * (float)(tex->width  - 1);
    float fy = v * (float)(tex->height - 1);

    if (filter == FILTER_NEAREST) {
        int x = (int)(fx + 0.5f);
        int y = (int)(fy + 0.5f);
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= tex->width)  x = tex->width  - 1;
        if (y >= tex->height) y = tex->height - 1;
        return get_pixel(tex, x, y);
    }

    int x0 = (int)fx;
    int y0 = (int)fy;
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x0 >= tex->width)  x0 = tex->width  - 1;
    if (y0 >= tex->height) y0 = tex->height - 1;

    if (wrap == WRAP_REPEAT) {
        x1 = x1 % tex->width;
        y1 = y1 % tex->height;
        if (x1 < 0) x1 += tex->width;
        if (y1 < 0) y1 += tex->height;
    } else {
        if (x1 >= tex->width)  x1 = tex->width  - 1;
        if (y1 >= tex->height) y1 = tex->height - 1;
    }

    float tx = fx - (float)x0;
    float ty = fy - (float)y0;

    color_t c00 = get_pixel(tex, x0, y0);
    color_t c10 = get_pixel(tex, x1, y0);
    color_t c01 = get_pixel(tex, x0, y1);
    color_t c11 = get_pixel(tex, x1, y1);

    color_t c0 = lerp_color(c00, c10, tx);
    color_t c1 = lerp_color(c01, c11, tx);
    return lerp_color(c0, c1, ty);
}

#endif /* RASTERIZER_TEXTURE_H */