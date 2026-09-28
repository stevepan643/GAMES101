#ifndef RASTERIZER_PIPELINE_H
#define RASTERIZER_PIPELINE_H

#include "color.h"
#include "common.h"
#include "vec.h"

typedef struct program_t program_t;

typedef enum
{
    VERT_ATTR_FLOAT3,
    VERT_ATTR_FLOAT2,
} vertex_attr_type_t;

typedef struct
{
    uint32_t location;
    vertex_attr_type_t type;
    size_t offset;
} vertex_attr_t;

typedef struct
{
    const void *data;
    vertex_attr_type_t type;
} shader_attrib_t;

typedef struct
{
    shader_attrib_t *attribs;
    uint32_t attr_count;
} vertex_input_t;

typedef struct
{
    float *z_buffer;
    color_t *color_buffer;
    size_t sample_count;
    uint32_t w, h;
} render_target_t;

typedef void (*vertex_shader_fnt)(const vertex_input_t *, void *out, vec4f_t *out_position,
                                  void *uniform);
typedef void (*fragment_shader_fnt)(vec2f_t screen_pos, float depth, const void *in,
                                    color_t *out_color, void *uniform);

program_t *create_program(void);
void program_set_vertex_shader(program_t *p, vertex_shader_fnt vs);
void program_set_fragment_shader(program_t *p, fragment_shader_fnt fs);
void program_link(program_t *p, size_t varying_size);
void program_draw(program_t *p, render_target_t target, vertex_attr_t *attrs, size_t count,
                  size_t stride, void *v, size_t vcount, uint32_t *i, size_t icount, void *vuniform,
                  void *funiform);

vec2f_t program_location_get2f(const vertex_input_t *in, uint32_t location);
vec3f_t program_location_get3f(const vertex_input_t *in, uint32_t location);

#endif