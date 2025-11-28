#include "test_common.h"
#include "atmos_simd.h"

class SIMDTest : public ::testing::Test {};

TEST_F(SIMDTest, HorizontalAddSmall) {
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float result = simd_horizontal_add(arr, 4);
    EXPECT_NEAR(result, 10.0f, 0.001f);
}

TEST_F(SIMDTest, HorizontalAddLarge) {
    float arr[16];
    float expected = 0.0f;
    for (int i = 0; i < 16; i++) {
        arr[i] = (float)(i + 1);
        expected += arr[i];
    }
    
    float result = simd_horizontal_add(arr, 16);
    EXPECT_NEAR(result, expected, 0.001f);
}

TEST_F(SIMDTest, HorizontalAddNonAligned) {
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
    float result = simd_horizontal_add(arr, 7);
    EXPECT_NEAR(result, 28.0f, 0.001f);
}

TEST_F(SIMDTest, HorizontalAddEmpty) {
    float arr[] = {0.0f};
    float result = simd_horizontal_add(arr, 0);
    EXPECT_NEAR(result, 0.0f, 0.001f);
}

TEST_F(SIMDTest, DotProductSmall) {
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[] = {2.0f, 3.0f, 4.0f, 5.0f};
    
    float result = simd_dot_product(a, b, 4);
    float expected = 1*2 + 2*3 + 3*4 + 4*5;
    
    EXPECT_NEAR(result, expected, 0.001f);
}

TEST_F(SIMDTest, DotProductLarge) {
    float a[16], b[16];
    float expected = 0.0f;
    for (int i = 0; i < 16; i++) {
        a[i] = (float)(i + 1);
        b[i] = (float)(16 - i);
        expected += a[i] * b[i];
    }
    
    float result = simd_dot_product(a, b, 16);
    EXPECT_NEAR(result, expected, 0.01f);
}

TEST_F(SIMDTest, DotProductNonAligned) {
    float a[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float b[] = {5.0f, 4.0f, 3.0f, 2.0f, 1.0f};
    
    float result = simd_dot_product(a, b, 5);
    float expected = 1*5 + 2*4 + 3*3 + 4*2 + 5*1;
    
    EXPECT_NEAR(result, expected, 0.001f);
}

TEST_F(SIMDTest, AddArraysSmall) {
    float dst[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float src[] = {10.0f, 20.0f, 30.0f, 40.0f};
    
    simd_add_arrays(dst, src, 4);
    
    EXPECT_NEAR(dst[0], 11.0f, 0.001f);
    EXPECT_NEAR(dst[1], 22.0f, 0.001f);
    EXPECT_NEAR(dst[2], 33.0f, 0.001f);
    EXPECT_NEAR(dst[3], 44.0f, 0.001f);
}

TEST_F(SIMDTest, AddArraysLarge) {
    float dst[16], src[16];
    for (int i = 0; i < 16; i++) {
        dst[i] = (float)i;
        src[i] = (float)(100 + i);
    }
    
    simd_add_arrays(dst, src, 16);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_NEAR(dst[i], (float)(100 + 2*i), 0.001f);
    }
}

TEST_F(SIMDTest, SubArraysSmall) {
    float dst[] = {10.0f, 20.0f, 30.0f, 40.0f};
    float src[] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    simd_sub_arrays(dst, src, 4);
    
    EXPECT_NEAR(dst[0], 9.0f, 0.001f);
    EXPECT_NEAR(dst[1], 18.0f, 0.001f);
    EXPECT_NEAR(dst[2], 27.0f, 0.001f);
    EXPECT_NEAR(dst[3], 36.0f, 0.001f);
}

TEST_F(SIMDTest, MulScalarSmall) {
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f};
    
    simd_mul_scalar(arr, 2.5f, 4);
    
    EXPECT_NEAR(arr[0], 2.5f, 0.001f);
    EXPECT_NEAR(arr[1], 5.0f, 0.001f);
    EXPECT_NEAR(arr[2], 7.5f, 0.001f);
    EXPECT_NEAR(arr[3], 10.0f, 0.001f);
}

TEST_F(SIMDTest, MulScalarLarge) {
    float arr[16];
    for (int i = 0; i < 16; i++) {
        arr[i] = (float)(i + 1);
    }
    
    simd_mul_scalar(arr, 3.0f, 16);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_NEAR(arr[i], (float)((i + 1) * 3), 0.001f);
    }
}

TEST_F(SIMDTest, CopyArrays) {
    float src[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float dst[8] = {0};
    
    simd_copy(dst, src, 8);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_NEAR(dst[i], src[i], 0.001f);
    }
}

TEST_F(SIMDTest, CopyArraysLarge) {
    float src[16], dst[16] = {0};
    for (int i = 0; i < 16; i++) {
        src[i] = (float)(i * 7);
    }
    
    simd_copy(dst, src, 16);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_NEAR(dst[i], src[i], 0.001f);
    }
}

TEST_F(SIMDTest, ZeroArray) {
    float arr[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    
    simd_zero(arr, 8);
    
    for (int i = 0; i < 8; i++) {
        EXPECT_NEAR(arr[i], 0.0f, 0.001f);
    }
}

TEST_F(SIMDTest, ZeroArrayLarge) {
    float arr[16];
    for (int i = 0; i < 16; i++) {
        arr[i] = (float)i;
    }
    
    simd_zero(arr, 16);
    
    for (int i = 0; i < 16; i++) {
        EXPECT_NEAR(arr[i], 0.0f, 0.001f);
    }
}

TEST_F(SIMDTest, ClampLower) {
    EXPECT_NEAR(simd_clamp(-5.0f, 0.0f, 10.0f), 0.0f, 0.001f);
}

TEST_F(SIMDTest, ClampUpper) {
    EXPECT_NEAR(simd_clamp(15.0f, 0.0f, 10.0f), 10.0f, 0.001f);
}

TEST_F(SIMDTest, ClampMiddle) {
    EXPECT_NEAR(simd_clamp(5.0f, 0.0f, 10.0f), 5.0f, 0.001f);
}

TEST_F(SIMDTest, Max) {
    EXPECT_NEAR(simd_max(5.0f, 3.0f), 5.0f, 0.001f);
    EXPECT_NEAR(simd_max(3.0f, 5.0f), 5.0f, 0.001f);
    EXPECT_NEAR(simd_max(-5.0f, -3.0f), -3.0f, 0.001f);
}

TEST_F(SIMDTest, Min) {
    EXPECT_NEAR(simd_min(5.0f, 3.0f), 3.0f, 0.001f);
    EXPECT_NEAR(simd_min(3.0f, 5.0f), 3.0f, 0.001f);
    EXPECT_NEAR(simd_min(-5.0f, -3.0f), -5.0f, 0.001f);
}

TEST_F(SIMDTest, Abs) {
    EXPECT_NEAR(simd_abs(5.0f), 5.0f, 0.001f);
    EXPECT_NEAR(simd_abs(-5.0f), 5.0f, 0.001f);
    EXPECT_NEAR(simd_abs(0.0f), 0.0f, 0.001f);
}

TEST_F(SIMDTest, LargeArrayOperations) {
    const int size = 1000;
    float a[size], b[size];
    
    for (int i = 0; i < size; i++) {
        a[i] = (float)i;
        b[i] = (float)(size - i);
    }
    
    float sum = simd_horizontal_add(a, size);
    float expectedSum = (size - 1) * size / 2.0f;
    EXPECT_NEAR(sum, expectedSum, 1.0f);
    
    float dot = simd_dot_product(a, b, size);
    EXPECT_GT(dot, 0.0f);
}

TEST_F(SIMDTest, PerformanceComparison) {
    const int size = 10000;
    const int iterations = 1000;
    
    std::vector<float> arr(size);
    for (int i = 0; i < size; i++) {
        arr[i] = (float)i;
    }
    
    PerformanceTimer timer;
    
    timer.Start();
    volatile float simdResult = 0;
    for (int i = 0; i < iterations; i++) {
        simdResult = simd_horizontal_add(arr.data(), size);
    }
    double simdTime = timer.ElapsedMs();
    
    timer.Start();
    volatile float scalarResult = 0;
    for (int iter = 0; iter < iterations; iter++) {
        float sum = 0;
        for (int i = 0; i < size; i++) {
            sum += arr[i];
        }
        scalarResult = sum;
    }
    double scalarTime = timer.ElapsedMs();
    
    EXPECT_NEAR((float)simdResult, (float)scalarResult, 1.0f);
    std::cout << "SIMD time: " << simdTime << " ms, Scalar time: " << scalarTime << " ms" << std::endl;
}

TEST_F(SIMDTest, EdgeCaseSingleElement) {
    float arr[] = {42.0f};
    
    EXPECT_NEAR(simd_horizontal_add(arr, 1), 42.0f, 0.001f);
    
    simd_mul_scalar(arr, 2.0f, 1);
    EXPECT_NEAR(arr[0], 84.0f, 0.001f);
    
    simd_zero(arr, 1);
    EXPECT_NEAR(arr[0], 0.0f, 0.001f);
}

TEST_F(SIMDTest, NegativeValues) {
    float arr[] = {-1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f};
    
    float sum = simd_horizontal_add(arr, 8);
    EXPECT_NEAR(sum, -36.0f, 0.001f);
    
    simd_mul_scalar(arr, -1.0f, 8);
    for (int i = 0; i < 8; i++) {
        EXPECT_GT(arr[i], 0.0f);
    }
}

TEST_F(SIMDTest, MixedValues) {
    float a[] = {-1.0f, 2.0f, -3.0f, 4.0f, -5.0f, 6.0f, -7.0f, 8.0f};
    float b[] = {1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f};
    
    float dot = simd_dot_product(a, b, 8);
    float expected = -1 - 4 - 9 - 16 - 25 - 36 - 49 - 64;
    
    EXPECT_NEAR(dot, expected, 0.001f);
}