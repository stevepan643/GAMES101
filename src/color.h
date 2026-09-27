#ifndef RASTERIZATION_COLOR_H
#define RASTERIZATION_COLOR_H

#include "common.h"

typedef struct { uint8_t r, g, b, a; } color_t;

#define COLOR_RGB(rv,gv,bv)      ((color_t){ .r=(rv), .g=(gv), .b=(bv), .a=255 })
#define COLOR_RGBA(rv,gv,bv,av)   ((color_t){ .r=(rv), .g=(gv), .b=(bv), .a=(av) })

#endif  /* RASTERIZATION_COLOR_H */