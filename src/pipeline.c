#include "pipeline.h"

#include "common.h"

struct program_t
{
    vertex_shader_fnt vs;
    fragment_shader_fnt fs;
    size_t varying_size;
    void *varying_buf;
    bool linked;
};

program_t *create_program(void)
{
    program_t *p = malloc(sizeof(program_t));
    if (!p)
        return NULL;
    p->vs = NULL;
    p->fs = NULL;
    p->varying_size = 0;
    p->varying_buf = NULL;
    p->linked = false;
    return p;
}
void program_set_vertex_shader(program_t *p, vertex_shader_fnt vs)
{
    p->vs = vs;
}
void program_set_fragment_shader(program_t *p, fragment_shader_fnt fs)
{
    p->fs = fs;
}
void program_link(program_t *p, size_t varying_size)
{
    p->varying_size = varying_size;
    if (p->varying_buf != NULL)
    {
        free(p->varying_buf);
        p->varying_buf = NULL;
    }
    p->linked = true;
}

void generate_sample_positions(uint32_t samples, float (*out)[2])
{
    static const float s1[1][2] = {{0.5f, 0.5f}};
    static const float s2[2][2] = {{0.25f, 0.5f}, {0.75f, 0.5f}};
    static const float s4[4][2] = {
        {0.375f, 0.125f}, {0.875f, 0.375f}, {0.125f, 0.625f}, {0.625f, 0.875f}};
    static const float s8[8][2] = {{0.375f, 0.125f}, {0.875f, 0.375f}, {0.125f, 0.625f},
                                   {0.625f, 0.875f}, {0.125f, 0.375f}, {0.375f, 0.875f},
                                   {0.625f, 0.125f}, {0.875f, 0.625f}};
    static const float s16[16][2] = {
        {0.125f, 0.125f}, {0.375f, 0.125f}, {0.625f, 0.125f}, {0.875f, 0.125f},
        {0.125f, 0.375f}, {0.375f, 0.375f}, {0.625f, 0.375f}, {0.875f, 0.375f},
        {0.125f, 0.625f}, {0.375f, 0.625f}, {0.625f, 0.625f}, {0.875f, 0.625f},
        {0.125f, 0.875f}, {0.375f, 0.875f}, {0.625f, 0.875f}, {0.875f, 0.875f}};

    switch (samples)
    {
        case 1:
            for (int i = 0; i < 1; i++)
            {
                out[i][0] = s1[i][0];
                out[i][1] = s1[i][1];
            }
            break;
        case 2:
            for (int i = 0; i < 2; i++)
            {
                out[i][0] = s2[i][0];
                out[i][1] = s2[i][1];
            }
            break;
        case 4:
            for (int i = 0; i < 4; i++)
            {
                out[i][0] = s4[i][0];
                out[i][1] = s4[i][1];
            }
            break;
        case 8:
            for (int i = 0; i < 8; i++)
            {
                out[i][0] = s8[i][0];
                out[i][1] = s8[i][1];
            }
            break;
        case 16:
            for (int i = 0; i < 16; i++)
            {
                out[i][0] = s16[i][0];
                out[i][1] = s16[i][1];
            }
            break;
        default:
        {
            uint32_t n = (uint32_t)ceilf(sqrtf((float)samples));
            for (uint32_t i = 0; i < samples; ++i)
            {
                uint32_t gx = i % n;
                uint32_t gy = i / n;
                out[i][0] = ((float)gx + 0.5f) / (float)n;
                out[i][1] = ((float)gy + 0.5f) / (float)n;
            }
        }
        break;
    }
}

static inline void barycentric(vec4f_t *v1, vec4f_t *v2, vec4f_t *v3, float x, float y, float *a,
                               float *b, float *c)
{
    float x1 = v1->x, y1 = v1->y;
    float x2 = v2->x, y2 = v2->y;
    float x3 = v3->x, y3 = v3->y;

    float area = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);

    if (area == 0.0f)
    {
        *a = *b = *c = 0.0f;
        return;
    }
    float inv = 1.0f / area;

    *a = ((x2 - x) * (y3 - y) - (y2 - y) * (x3 - x)) * inv;
    *b = ((x3 - x) * (y1 - y) - (y3 - y) * (x1 - x)) * inv;
    *c = 1.0f - *a - *b;
}

static inline vec4f_t perspective_divide(vec4f_t v)
{
    vec4f_t r = {v.x / v.w, v.y / v.w, v.z / v.w, v.w};
    return r;
}

static inline vec4f_t ndc_to_screen(vec4f_t ndc, uint32_t w, uint32_t h)
{
    vec4f_t s = {(ndc.x + 1.0f) * 0.5f * (float)w, (1.0f - ndc.y) * 0.5f * (float)h, ndc.z, ndc.w};
    return s;
}

void program_draw(program_t *p, render_target_t target, vertex_attr_t *attrs, size_t count,
                  size_t stride, void *v, size_t vcount, uint32_t *i, size_t icount, void *vuniform,
                  void *funiform)
{
    if (!p || !p->linked)
        return;
    if (!i || icount < 3)
        return;

    uint32_t w = target.w, h = target.h;
    uint32_t samples = (uint32_t)target.sample_count;
    if (samples == 0)
        samples = 1;

    if (p->varying_buf != NULL)
        free(p->varying_buf);
    p->varying_buf = p->varying_size ? malloc(p->varying_size * vcount) : NULL;
    uint8_t *vbuf = (uint8_t *)p->varying_buf;

    vec4f_t *positions = malloc(sizeof(vec4f_t) * vcount);

    shader_attrib_t *sa = malloc(count * sizeof(shader_attrib_t));

    for (size_t vi = 0; vi < vcount; vi++)
    {
        const uint8_t *base = (const uint8_t *)v + vi * stride;

        for (size_t ai = 0; ai < count; ai++)
        {
            sa[attrs[ai].location].type = attrs[ai].type;
            sa[attrs[ai].location].data = base + attrs[ai].offset;
        }

        vertex_input_t vin = {sa, (uint32_t)count};
        vec4f_t clip;
        p->vs(&vin, vbuf + vi * p->varying_size, &clip, vuniform);

        vec4f_t ndc = perspective_divide(clip);
        positions[vi] = ndc_to_screen(ndc, w, h);
    }

    free(sa);

    static float (*cached_sample_pos)[2] = NULL;
    static uint32_t cached_samples = 0;
    if (samples != cached_samples)
    {
        free(cached_sample_pos);
        cached_sample_pos = malloc(sizeof(float) * 2 * samples);
        generate_sample_positions(samples, cached_sample_pos);
        cached_samples = samples;
    }
    float (*sample_positions)[2] = cached_sample_pos;

    size_t nf = p->varying_size / sizeof(float);
    float *frag_varying = nf ? malloc(nf * sizeof(float)) : NULL;

    for (size_t ti = 0; ti < icount; ti += 3)
    {
        vec4f_t *v1 = &positions[i[ti]];
        vec4f_t *v2 = &positions[i[ti + 1]];
        vec4f_t *v3 = &positions[i[ti + 2]];

        float *varing1 = (float *)(vbuf + i[ti] * p->varying_size);
        float *varing2 = (float *)(vbuf + i[ti + 1] * p->varying_size);
        float *varing3 = (float *)(vbuf + i[ti + 2] * p->varying_size);

        float min_xf = fminf(fminf(v1->x, v2->x), v3->x);
        float max_xf = fmaxf(fmaxf(v1->x, v2->x), v3->x);
        float min_yf = fminf(fminf(v1->y, v2->y), v3->y);
        float max_yf = fmaxf(fmaxf(v1->y, v2->y), v3->y);

        int min_x = (int)floorf(min_xf);
        int max_x = (int)ceilf(max_xf);
        int min_y = (int)floorf(min_yf);
        int max_y = (int)ceilf(max_yf);

        if (min_x < 0)
            min_x = 0;
        if (min_y < 0)
            min_y = 0;
        if (max_x >= (int)w)
            max_x = (int)w - 1;
        if (max_y >= (int)h)
            max_y = (int)h - 1;

        for (int y = min_y; y <= max_y; ++y)
        {
            for (int x = min_x; x <= max_x; ++x)
            {
                for (uint32_t si = 0; si < samples; ++si)
                {
                    float sx = (float)x + sample_positions[si][0];
                    float sy = (float)y + sample_positions[si][1];

                    float a, b, c;
                    barycentric(v1, v2, v3, sx, sy, &a, &b, &c);
                    if (a < 0.0f || b < 0.0f || c < 0.0f)
                        continue;

                    float z = a * v1->z + b * v2->z + c * v3->z;

                    size_t idx = ((size_t)y * w + (size_t)x) * samples + si;
                    if (z >= target.z_buffer[idx])
                        continue;

                    if (nf)
                    {
                        float p1 = a / v1->w;
                        float p2 = b / v2->w;
                        float p3 = c / v3->w;
                        float inv = 1.0f / (p1 + p2 + p3);
                        p1 *= inv;
                        p2 *= inv;
                        p3 *= inv;

                        for (size_t k = 0; k < nf; ++k)
                            frag_varying[k] = varing1[k] * p1 + varing2[k] * p2 + varing3[k] * p3;
                    }

                    vec2f_t screen_pos = {sx, sy};
                    color_t out_color = COLOR_RGB(0, 0, 0);
                    p->fs(screen_pos, z, nf ? frag_varying : NULL, &out_color, funiform);

                    target.z_buffer[idx] = z;
                    target.color_buffer[idx] = out_color;
                }
            }
        }
    }

    free(frag_varying);
    free(positions);
}

vec3f_t program_location_get3f(const vertex_input_t *in, uint32_t location)
{
    const float *f = (const float *)in->attribs[location].data;
    return (vec3f_t){f[0], f[1], f[2]};
}

vec2f_t program_location_get2f(const vertex_input_t *in, uint32_t location)
{
    const float *f = (const float *)in->attribs[location].data;
    return (vec2f_t){f[0], f[1]};
}