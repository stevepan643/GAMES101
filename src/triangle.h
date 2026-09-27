#ifndef RASTERIZER_TRIANGLE_H
#define RASTERIZER_TRIANGLE_H

#include "color.h"
#include "vec.h"

typedef struct
{
    vec4f_t v1;
    vec4f_t v2;
    vec4f_t v3;
    color_t c1;
    color_t c2;
    color_t c3;
} triangle_t;

static inline triangle_t triangle_create3f(vec3f_t v1, vec3f_t v2, vec3f_t v3, color_t c1,
                                           color_t c2, color_t c3)
{
    triangle_t t;
    t.v1 = (vec4f_t){v1.x, v1.y, v1.z, 1.0f};
    t.v2 = (vec4f_t){v2.x, v2.y, v2.z, 1.0f};
    t.v3 = (vec4f_t){v3.x, v3.y, v3.z, 1.0f};
    t.c1 = c1;
    t.c2 = c2;
    t.c3 = c3;
    return t;
}

static inline triangle_t triangle_create4f(vec4f_t v1, vec4f_t v2, vec4f_t v3, color_t c1,
                                           color_t c2, color_t c3)
{
    triangle_t t;
    t.v1 = v1;
    t.v2 = v2;
    t.v3 = v3;
    t.c1 = c1;
    t.c2 = c2;
    t.c3 = c3;
    return t;
}

#endif /* RASTERIZER_TRIANGLE_H */