#ifndef RASTERIZATION_COMPUTE_H
#define RASTERIZATION_COMPUTE_H

#include <math.h>

#include "vec.h"
#include "matrix.h"

/* vec2f_t */
static inline vec2f_t vec2f_add(vec2f_t a, vec2f_t b)
{
    vec2f_t r = { a.x + b.x, a.y + b.y };
    return r;
}

static inline vec2f_t vec2f_sub(vec2f_t a, vec2f_t b)
{
    vec2f_t r = { a.x - b.x, a.y - b.y };
    return r;
}

static inline vec2f_t vec2f_scale(vec2f_t v, float s)
{
    vec2f_t r = { v.x * s, v.y * s };
    return r;
}

static inline float vec2f_dot(vec2f_t a, vec2f_t b)
{
    return a.x * b.x + a.y * b.y;
}

static inline float vec2f_length(vec2f_t v)
{
    return sqrtf(vec2f_dot(v, v));
}

static inline vec2f_t vec2f_normalize(vec2f_t v)
{
    float len = vec2f_length(v);
    if (len == 0.0f) {
        vec2f_t zero = { 0.0f, 0.0f };
        return zero;
    }
    return vec2f_scale(v, 1.0f / len);
}

/* vec3f_t */
static inline vec3f_t vec3f_add(vec3f_t a, vec3f_t b)
{
    vec3f_t r = { a.x + b.x, a.y + b.y, a.z + b.z };
    return r;
}

static inline vec3f_t vec3f_sub(vec3f_t a, vec3f_t b)
{
    vec3f_t r = { a.x - b.x, a.y - b.y, a.z - b.z };
    return r;
}

static inline vec3f_t vec3f_scale(vec3f_t v, float s)
{
    vec3f_t r = { v.x * s, v.y * s, v.z * s };
    return r;
}

static inline float vec3f_dot(vec3f_t a, vec3f_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3f_t vec3f_cross(vec3f_t a, vec3f_t b)
{
    vec3f_t r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

static inline float vec3f_length(vec3f_t v)
{
    return sqrtf(vec3f_dot(v, v));
}

static inline vec3f_t vec3f_normalize(vec3f_t v)
{
    float len = vec3f_length(v);
    if (len == 0.0f) {
        vec3f_t zero = { 0.0f, 0.0f, 0.0f };
        return zero;
    }
    return vec3f_scale(v, 1.0f / len);
}

/* vec4f_t */
static inline vec4f_t vec4f_add(vec4f_t a, vec4f_t b)
{
    vec4f_t r = { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    return r;
}

static inline vec4f_t vec4f_sub(vec4f_t a, vec4f_t b)
{
    vec4f_t r = { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    return r;
}

static inline vec4f_t vec4f_scale(vec4f_t v, float s)
{
    vec4f_t r = { v.x * s, v.y * s, v.z * s, v.w * s };
    return r;
}

static inline float vec4f_dot(vec4f_t a, vec4f_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline float vec4f_length(vec4f_t v)
{
    return sqrtf(vec4f_dot(v, v));
}

static inline vec4f_t vec4f_normalize(vec4f_t v)
{
    float len = vec4f_length(v);
    if (len == 0.0f) {
        vec4f_t zero = { 0.0f, 0.0f, 0.0f, 0.0f };
        return zero;
    }
    return vec4f_scale(v, 1.0f / len);
}

static inline mat4f_t mat4f_identity(void)
{
    mat4f_t m = { {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f }
    } };
    return m;
}

static inline mat4f_t mat4f_add(mat4f_t a, mat4f_t b)
{
    mat4f_t r;
    int i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            r.m[i][j] = a.m[i][j] + b.m[i][j];
    return r;
}

static inline mat4f_t mat4f_sub(mat4f_t a, mat4f_t b)
{
    mat4f_t r;
    int i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            r.m[i][j] = a.m[i][j] - b.m[i][j];
    return r;
}

static inline mat4f_t mat4f_scale(mat4f_t a, float s)
{
    mat4f_t r;
    int i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            r.m[i][j] = a.m[i][j] * s;
    return r;
}

static inline mat4f_t mat4f_mul(mat4f_t a, mat4f_t b)
{
    mat4f_t r;
    int i, j, k;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            float sum = 0.0f;
            for (k = 0; k < 4; ++k)
                sum += a.m[i][k] * b.m[k][j];
            r.m[i][j] = sum;
        }
    }
    return r;
}

static inline vec4f_t mat4f_mul_vec4f(mat4f_t m, vec4f_t v)
{
    vec4f_t r;
    r.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    r.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    r.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    r.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
    return r;
}

static inline mat4f_t mat4f_transpose(mat4f_t m)
{
    mat4f_t r;
    int i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            r.m[i][j] = m.m[j][i];
    return r;
}

static inline int mat4f_inverse(mat4f_t m, mat4f_t *out)
{
    float inv[16];
    const float *a = (const float *)m.m;
    float det;
    int i;

    inv[0]  =  a[5] * a[10] * a[15] - a[5] * a[11] * a[14]
             - a[9] * a[6]  * a[15] + a[9] * a[7]  * a[14]
             + a[13] * a[6] * a[11] - a[13] * a[7] * a[10];
    inv[4]  = -a[4] * a[10] * a[15] + a[4] * a[11] * a[14]
             + a[8] * a[6]  * a[15] - a[8] * a[7]  * a[14]
             - a[12] * a[6] * a[11] + a[12] * a[7] * a[10];
    inv[8]  =  a[4] * a[9]  * a[15] - a[4] * a[11] * a[13]
             - a[8] * a[5]  * a[15] + a[8] * a[7]  * a[13]
             + a[12] * a[5] * a[11] - a[12] * a[7] * a[9];
    inv[12] = -a[4] * a[9]  * a[14] + a[4] * a[10] * a[13]
             + a[8] * a[5]  * a[14] - a[8] * a[6]  * a[13]
             - a[12] * a[5] * a[10] + a[12] * a[6] * a[9];
    inv[1]  = -a[1] * a[10] * a[15] + a[1] * a[11] * a[14]
             + a[9] * a[2]  * a[15] - a[9] * a[3]  * a[14]
             - a[13] * a[2] * a[11] + a[13] * a[3] * a[10];
    inv[5]  =  a[0] * a[10] * a[15] - a[0] * a[11] * a[14]
             - a[8] * a[2]  * a[15] + a[8] * a[3]  * a[14]
             + a[12] * a[2] * a[11] - a[12] * a[3] * a[10];
    inv[9]  = -a[0] * a[9]  * a[15] + a[0] * a[11] * a[13]
             + a[8] * a[1]  * a[15] - a[8] * a[3]  * a[13]
             - a[12] * a[1] * a[11] + a[12] * a[3] * a[9];
    inv[13] =  a[0] * a[9]  * a[14] - a[0] * a[10] * a[13]
             - a[8] * a[1]  * a[14] + a[8] * a[2]  * a[13]
             + a[12] * a[1] * a[10] - a[12] * a[2] * a[9];
    inv[2]  =  a[1] * a[6]  * a[15] - a[1] * a[7]  * a[14]
             - a[5] * a[2]  * a[15] + a[5] * a[3]  * a[14]
             + a[13] * a[2] * a[7]  - a[13] * a[3] * a[6];
    inv[6]  = -a[0] * a[6]  * a[15] + a[0] * a[7]  * a[14]
             + a[4] * a[2]  * a[15] - a[4] * a[3]  * a[14]
             - a[12] * a[2] * a[7]  + a[12] * a[3] * a[6];
    inv[10] =  a[0] * a[5]  * a[15] - a[0] * a[7]  * a[13]
             - a[4] * a[1]  * a[15] + a[4] * a[3]  * a[13]
             + a[12] * a[1] * a[7]  - a[12] * a[3] * a[5];
    inv[14] = -a[0] * a[5]  * a[14] + a[0] * a[6]  * a[13]
             + a[4] * a[1]  * a[14] - a[4] * a[2]  * a[13]
             - a[12] * a[1] * a[6]  + a[12] * a[2] * a[5];
    inv[3]  = -a[1] * a[6]  * a[11] + a[1] * a[7]  * a[10]
             + a[5] * a[2]  * a[11] - a[5] * a[3]  * a[10]
             - a[9]  * a[2] * a[7]  + a[9]  * a[3] * a[6];
    inv[7]  =  a[0] * a[6]  * a[11] - a[0] * a[7]  * a[10]
             - a[4] * a[2]  * a[11] + a[4] * a[3]  * a[10]
             + a[8]  * a[2] * a[7]  - a[8]  * a[3] * a[6];
    inv[11] = -a[0] * a[5]  * a[11] + a[0] * a[7]  * a[9]
             + a[4] * a[1]  * a[11] - a[4] * a[3]  * a[9]
             - a[8]  * a[1] * a[7]  + a[8]  * a[3] * a[5];
    inv[15] =  a[0] * a[5]  * a[10] - a[0] * a[6]  * a[9]
             - a[4] * a[1]  * a[10] + a[4] * a[2]  * a[9]
             + a[8]  * a[1] * a[6]  - a[8]  * a[2] * a[5];

    det = a[0] * inv[0] + a[1] * inv[4] + a[2] * inv[8] + a[3] * inv[12];
    if (det == 0.0f)
        return 0;

    det = 1.0f / det;
    for (i = 0; i < 16; ++i)
        ((float *)out->m)[i] = inv[i] * det;
    return 1;
}

static inline mat4f_t mat4f_translate(float tx, float ty, float tz)
{
    mat4f_t m = mat4f_identity();
    m.m[0][3] = tx;
    m.m[1][3] = ty;
    m.m[2][3] = tz;
    return m;
}

static inline mat4f_t mat4f_scale_m(float sx, float sy, float sz)
{
    mat4f_t m = mat4f_identity();
    m.m[0][0] = sx;
    m.m[1][1] = sy;
    m.m[2][2] = sz;
    return m;
}

static inline mat4f_t mat4f_rotate_x(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    mat4f_t m = mat4f_identity();
    m.m[1][1] =  c;
    m.m[1][2] = -s;
    m.m[2][1] =  s;
    m.m[2][2] =  c;
    return m;
}

static inline mat4f_t mat4f_rotate_y(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    mat4f_t m = mat4f_identity();
    m.m[0][0] =  c;
    m.m[0][2] =  s;
    m.m[2][0] = -s;
    m.m[2][2] =  c;
    return m;
}

static inline mat4f_t mat4f_rotate_z(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    mat4f_t m = mat4f_identity();
    m.m[0][0] =  c;
    m.m[0][1] = -s;
    m.m[1][0] =  s;
    m.m[1][1] =  c;
    return m;
}

#endif  /* RASTERIZATION_COMPUTE_H */