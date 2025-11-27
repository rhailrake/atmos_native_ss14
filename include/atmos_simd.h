#pragma once

#include <immintrin.h>
#include <math.h>

#ifdef _MSC_VER
#define ATMOS_INLINE __forceinline
#else
#define ATMOS_INLINE __attribute__((always_inline)) inline
#endif

ATMOS_INLINE float hsum256_ps(__m256 v)
{
    __m128 vlow = _mm256_castps256_ps128(v);
    __m128 vhigh = _mm256_extractf128_ps(v, 1);
    vlow = _mm_add_ps(vlow, vhigh);
    __m128 shuf = _mm_movehdup_ps(vlow);
    __m128 sums = _mm_add_ps(vlow, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

ATMOS_INLINE float hsum128_ps(__m128 v)
{
    __m128 shuf = _mm_movehdup_ps(v);
    __m128 sums = _mm_add_ps(v, shuf);
    shuf = _mm_movehl_ps(shuf, sums);
    sums = _mm_add_ss(sums, shuf);
    return _mm_cvtss_f32(sums);
}

ATMOS_INLINE float simd_horizontal_add(const float* arr, int count)
{
    float sum = 0.0f;
    int i = 0;

    for (; i + 8 <= count; i += 8)
    {
        __m256 v = _mm256_loadu_ps(arr + i);
        sum += hsum256_ps(v);
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 v = _mm_loadu_ps(arr + i);
        sum += hsum128_ps(v);
    }

    for (; i < count; i++)
        sum += arr[i];

    return sum;
}

ATMOS_INLINE float simd_dot_product(const float* a, const float* b, int count)
{
    float sum = 0.0f;
    int i = 0;

    for (; i + 8 <= count; i += 8)
    {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 prod = _mm256_mul_ps(va, vb);
        sum += hsum256_ps(prod);
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 prod = _mm_mul_ps(va, vb);
        sum += hsum128_ps(prod);
    }

    for (; i < count; i++)
        sum += a[i] * b[i];

    return sum;
}

ATMOS_INLINE void simd_add_arrays(float* dst, const float* src, int count)
{
    int i = 0;

    for (; i + 8 <= count; i += 8)
    {
        __m256 vd = _mm256_loadu_ps(dst + i);
        __m256 vs = _mm256_loadu_ps(src + i);
        _mm256_storeu_ps(dst + i, _mm256_add_ps(vd, vs));
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 vd = _mm_loadu_ps(dst + i);
        __m128 vs = _mm_loadu_ps(src + i);
        _mm_storeu_ps(dst + i, _mm_add_ps(vd, vs));
    }

    for (; i < count; i++)
        dst[i] += src[i];
}

ATMOS_INLINE void simd_sub_arrays(float* dst, const float* src, int count)
{
    int i = 0;

    for (; i + 8 <= count; i += 8)
    {
        __m256 vd = _mm256_loadu_ps(dst + i);
        __m256 vs = _mm256_loadu_ps(src + i);
        _mm256_storeu_ps(dst + i, _mm256_sub_ps(vd, vs));
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 vd = _mm_loadu_ps(dst + i);
        __m128 vs = _mm_loadu_ps(src + i);
        _mm_storeu_ps(dst + i, _mm_sub_ps(vd, vs));
    }

    for (; i < count; i++)
        dst[i] -= src[i];
}

ATMOS_INLINE void simd_mul_scalar(float* arr, float scalar, int count)
{
    int i = 0;
    __m256 vs256 = _mm256_set1_ps(scalar);
    __m128 vs128 = _mm_set1_ps(scalar);

    for (; i + 8 <= count; i += 8)
    {
        __m256 v = _mm256_loadu_ps(arr + i);
        _mm256_storeu_ps(arr + i, _mm256_mul_ps(v, vs256));
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 v = _mm_loadu_ps(arr + i);
        _mm_storeu_ps(arr + i, _mm_mul_ps(v, vs128));
    }

    for (; i < count; i++)
        arr[i] *= scalar;
}

ATMOS_INLINE void simd_copy(float* dst, const float* src, int count)
{
    int i = 0;

    for (; i + 8 <= count; i += 8)
    {
        __m256 v = _mm256_loadu_ps(src + i);
        _mm256_storeu_ps(dst + i, v);
    }

    for (; i + 4 <= count; i += 4)
    {
        __m128 v = _mm_loadu_ps(src + i);
        _mm_storeu_ps(dst + i, v);
    }

    for (; i < count; i++)
        dst[i] = src[i];
}

ATMOS_INLINE void simd_zero(float* arr, int count)
{
    int i = 0;
    __m256 zero256 = _mm256_setzero_ps();
    __m128 zero128 = _mm_setzero_ps();

    for (; i + 8 <= count; i += 8)
        _mm256_storeu_ps(arr + i, zero256);

    for (; i + 4 <= count; i += 4)
        _mm_storeu_ps(arr + i, zero128);

    for (; i < count; i++)
        arr[i] = 0.0f;
}

ATMOS_INLINE float simd_clamp(float val, float min_val, float max_val)
{
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

ATMOS_INLINE float simd_max(float a, float b)
{
    return a > b ? a : b;
}

ATMOS_INLINE float simd_min(float a, float b)
{
    return a < b ? a : b;
}

ATMOS_INLINE float simd_abs(float v)
{
    return v < 0 ? -v : v;
}