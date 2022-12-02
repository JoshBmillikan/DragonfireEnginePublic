//
// Created by josh on 12/1/22.
// A C++ port of K.jpg's OpenSimplex 2, smooth variant ("SuperSimplex")
// https://github.com/KdotJPG/OpenSimplex2
//

#include "noise.h"
#include "util.h"
#include <numbers>

namespace df {

static constexpr Long PRIME_X = 0x5205402B9270C86FL, PRIME_Y = 0x598CD327003817B5L, PRIME_Z = 0x5BCC226E9FA0BACBL,
                      PRIME_W = 0x56CC5227E58F554BL, HASH_MUL = 0x53A3F72DEEC546F5L;
static constexpr Long SEED_FLIP_3D = -0x52D547B2E96ED629L;
static constexpr float SKEW_2D = 0.366025403784439, UNSKEW_2D = -0.21132486540518713, SKEW_4D = 0.309016994374947f,
                       UNSKEW_4D = -0.138196601125011f, NORM_2D = 0.05481866495625118, NORM_3D = 0.2781926117527186,
                       NORM_4D = 0.11127401889945551, ROTATE3_ORTHOGONALIZER = UNSKEW_2D, RSQUARED_2D = 2.0f / 3.0f,
                       RSQUARED_3D = 3.0f / 4.0f, RSQUARED_4D = 4.0f / 5.0f;

static constexpr Int N_GRADS_2D_EXPONENT = 7, N_GRADS_3D_EXPONENT = 8, N_GRADS_4D_EXPONENT = 9,
                     N_GRADS_2D = 1 << N_GRADS_2D_EXPONENT, N_GRADS_3D = 1 << N_GRADS_3D_EXPONENT,
                     N_GRADS_4D = 1 << N_GRADS_4D_EXPONENT;

constexpr void initGrad2D(float* array)
{
    // clang-format off
    float grad2D[] = {
            0.38268343236509f,   0.923879532511287f,
            0.923879532511287f,  0.38268343236509f,
            0.923879532511287f, -0.38268343236509f,
            0.38268343236509f,  -0.923879532511287f,
            -0.38268343236509f,  -0.923879532511287f,
            -0.923879532511287f, -0.38268343236509f,
            -0.923879532511287f,  0.38268343236509f,
            -0.38268343236509f,   0.923879532511287f,
            //-------------------------------------//
            0.130526192220052f,  0.99144486137381f,
            0.608761429008721f,  0.793353340291235f,
            0.793353340291235f,  0.608761429008721f,
            0.99144486137381f,   0.130526192220051f,
            0.99144486137381f,  -0.130526192220051f,
            0.793353340291235f, -0.60876142900872f,
            0.608761429008721f, -0.793353340291235f,
            0.130526192220052f, -0.99144486137381f,
            -0.130526192220052f, -0.99144486137381f,
            -0.608761429008721f, -0.793353340291235f,
            -0.793353340291235f, -0.608761429008721f,
            -0.99144486137381f,  -0.130526192220052f,
            -0.99144486137381f,   0.130526192220051f,
            -0.793353340291235f,  0.608761429008721f,
            -0.608761429008721f,  0.793353340291235f,
            -0.130526192220052f,  0.99144486137381f,
    };
    // clang-format on
    for (float& grad : grad2D)
        grad /= NORM_2D;
    for (UInt i = 0, j = 0; i < N_GRADS_2D * 2; i++, j++) {
        if (j == sizeof(grad2D) / 4)
            j = 0;
        array[i] = grad2D[j];
    }
}

constexpr void initGrad3D(float* array)
{
    // clang-format off
    float grad3D[] = {
            2.22474487139f,       2.22474487139f,      -1.0f,                 0.0f,
            2.22474487139f,       2.22474487139f,       1.0f,                 0.0f,
            3.0862664687972017f,  1.1721513422464978f,  0.0f,                 0.0f,
            1.1721513422464978f,  3.0862664687972017f,  0.0f,                 0.0f,
            -2.22474487139f,       2.22474487139f,      -1.0f,                 0.0f,
            -2.22474487139f,       2.22474487139f,       1.0f,                 0.0f,
            -1.1721513422464978f,  3.0862664687972017f,  0.0f,                 0.0f,
            -3.0862664687972017f,  1.1721513422464978f,  0.0f,                 0.0f,
            -1.0f,                -2.22474487139f,      -2.22474487139f,       0.0f,
            1.0f,                -2.22474487139f,      -2.22474487139f,       0.0f,
            0.0f,                -3.0862664687972017f, -1.1721513422464978f,  0.0f,
            0.0f,                -1.1721513422464978f, -3.0862664687972017f,  0.0f,
            -1.0f,                -2.22474487139f,       2.22474487139f,       0.0f,
            1.0f,                -2.22474487139f,       2.22474487139f,       0.0f,
            0.0f,                -1.1721513422464978f,  3.0862664687972017f,  0.0f,
            0.0f,                -3.0862664687972017f,  1.1721513422464978f,  0.0f,
            //--------------------------------------------------------------------//
            -2.22474487139f,      -2.22474487139f,      -1.0f,                 0.0f,
            -2.22474487139f,      -2.22474487139f,       1.0f,                 0.0f,
            -3.0862664687972017f, -1.1721513422464978f,  0.0f,                 0.0f,
            -1.1721513422464978f, -3.0862664687972017f,  0.0f,                 0.0f,
            -2.22474487139f,      -1.0f,                -2.22474487139f,       0.0f,
            -2.22474487139f,       1.0f,                -2.22474487139f,       0.0f,
            -1.1721513422464978f,  0.0f,                -3.0862664687972017f,  0.0f,
            -3.0862664687972017f,  0.0f,                -1.1721513422464978f,  0.0f,
            -2.22474487139f,      -1.0f,                 2.22474487139f,       0.0f,
            -2.22474487139f,       1.0f,                 2.22474487139f,       0.0f,
            -3.0862664687972017f,  0.0f,                 1.1721513422464978f,  0.0f,
            -1.1721513422464978f,  0.0f,                 3.0862664687972017f,  0.0f,
            -1.0f,                 2.22474487139f,      -2.22474487139f,       0.0f,
            1.0f,                 2.22474487139f,      -2.22474487139f,       0.0f,
            0.0f,                 1.1721513422464978f, -3.0862664687972017f,  0.0f,
            0.0f,                 3.0862664687972017f, -1.1721513422464978f,  0.0f,
            -1.0f,                 2.22474487139f,       2.22474487139f,       0.0f,
            1.0f,                 2.22474487139f,       2.22474487139f,       0.0f,
            0.0f,                 3.0862664687972017f,  1.1721513422464978f,  0.0f,
            0.0f,                 1.1721513422464978f,  3.0862664687972017f,  0.0f,
            2.22474487139f,      -2.22474487139f,      -1.0f,                 0.0f,
            2.22474487139f,      -2.22474487139f,       1.0f,                 0.0f,
            1.1721513422464978f, -3.0862664687972017f,  0.0f,                 0.0f,
            3.0862664687972017f, -1.1721513422464978f,  0.0f,                 0.0f,
            2.22474487139f,      -1.0f,                -2.22474487139f,       0.0f,
            2.22474487139f,       1.0f,                -2.22474487139f,       0.0f,
            3.0862664687972017f,  0.0f,                -1.1721513422464978f,  0.0f,
            1.1721513422464978f,  0.0f,                -3.0862664687972017f,  0.0f,
            2.22474487139f,      -1.0f,                 2.22474487139f,       0.0f,
            2.22474487139f,       1.0f,                 2.22474487139f,       0.0f,
            1.1721513422464978f,  0.0f,                 3.0862664687972017f,  0.0f,
            3.0862664687972017f,  0.0f,                 1.1721513422464978f,  0.0f,
    };
    // clang-format on
    for (float& grad : grad3D)
        grad /= NORM_3D;
    for (UInt i = 0, j = 0; i < N_GRADS_3D * 4; i++, j++) {
        if (j == sizeof(grad3D) / 4)
            j = 0;
        array[i] = grad3D[j];
    }
}

static constinit struct GradiantLookupTable {
    float grad2D[N_GRADS_2D * 2]{}, grad3D[N_GRADS_3D * 4]{};

    constexpr GradiantLookupTable()
    {
        initGrad2D(grad2D);
        initGrad3D(grad3D);
    };
} GRADIENTS = GradiantLookupTable();

static float grad(ULong seed, Long xsvp, Long ysvp, float dx, float dy)
{
    Long hash = std::bit_cast<Long>(seed) ^ xsvp ^ ysvp;
    hash *= HASH_MUL;
    hash ^= hash >> (64 - N_GRADS_2D_EXPONENT + 1);
    int gi = (int) hash & ((N_GRADS_2D - 1) << 1);
    return GRADIENTS.grad2D[gi | 0] * dx + GRADIENTS.grad2D[gi | 1] * dy;
}

static float noise2UnskewedBase(const ULong seed, const float xs, const float ys)
{
    // Get base points and offsets.
    const Int xsb = intFloor(xs), ysb = intFloor(ys);
    const float xi = (xs - (float) xsb), yi = (ys - (float) ysb);

    // Prime pre-multiplication for hash.
    Long xsbp = xsb * PRIME_X, ysbp = ysb * PRIME_Y;

    // Unskew.
    float t = (xi + yi) * UNSKEW_2D;
    float dx0 = xi + t, dy0 = yi + t;

    // First vertex.
    float a0 = RSQUARED_2D - dx0 * dx0 - dy0 * dy0;
    float value = (a0 * a0) * (a0 * a0) * grad(seed, xsbp, ysbp, dx0, dy0);

    // Second vertex.
    float a1 = (float) (2 * (1 + 2 * UNSKEW_2D) * (1 / UNSKEW_2D + 2)) * t
               + ((float) (-2 * (1 + 2 * UNSKEW_2D) * (1 + 2 * UNSKEW_2D)) + a0);
    float dx1 = dx0 - (float) (1 + 2 * UNSKEW_2D);
    float dy1 = dy0 - (float) (1 + 2 * UNSKEW_2D);
    value += (a1 * a1) * (a1 * a1) * grad(seed, xsbp + PRIME_X, ysbp + PRIME_Y, dx1, dy1);

    // Third and fourth vertices.
    // Nested conditionals were faster than compact bit logic/arithmetic.
    float xmyi = xi - yi;
    if (t < UNSKEW_2D) {
        if (xi + xmyi > 1) {
            float dx2 = dx0 - (float)(3 * UNSKEW_2D + 2);
            float dy2 = dy0 - (float)(3 * UNSKEW_2D + 1);
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp + (PRIME_X << 1), ysbp + PRIME_Y, dx2, dy2);
            }
        }
        else
        {
            float dx2 = dx0 - (float)UNSKEW_2D;
            float dy2 = dy0 - (float)(UNSKEW_2D + 1);
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp, ysbp + PRIME_Y, dx2, dy2);
            }
        }

        if (yi - xmyi > 1) {
            float dx3 = dx0 - (float)(3 * UNSKEW_2D + 1);
            float dy3 = dy0 - (float)(3 * UNSKEW_2D + 2);
            float a3 = RSQUARED_2D - dx3 * dx3 - dy3 * dy3;
            if (a3 > 0) {
                value += (a3 * a3) * (a3 * a3) * grad(seed, xsbp + PRIME_X, ysbp + (PRIME_Y << 1), dx3, dy3);
            }
        }
        else
        {
            float dx3 = dx0 - (float)(UNSKEW_2D + 1);
            float dy3 = dy0 - (float)UNSKEW_2D;
            float a3 = RSQUARED_2D - dx3 * dx3 - dy3 * dy3;
            if (a3 > 0) {
                value += (a3 * a3) * (a3 * a3) * grad(seed, xsbp + PRIME_X, ysbp, dx3, dy3);
            }
        }
    }
    else
    {
        if (xi + xmyi < 0) {
            float dx2 = dx0 + (float)(1 + UNSKEW_2D);
            float dy2 = dy0 + (float)UNSKEW_2D;
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp - PRIME_X, ysbp, dx2, dy2);
            }
        }
        else
        {
            float dx2 = dx0 - (float)(UNSKEW_2D + 1);
            float dy2 = dy0 - (float)UNSKEW_2D;
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp + PRIME_X, ysbp, dx2, dy2);
            }
        }

        if (yi < xmyi) {
            float dx2 = dx0 + (float)UNSKEW_2D;
            float dy2 = dy0 + (float)(UNSKEW_2D + 1);
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp, ysbp - PRIME_Y, dx2, dy2);
            }
        }
        else
        {
            float dx2 = dx0 - (float)UNSKEW_2D;
            float dy2 = dy0 - (float)(UNSKEW_2D + 1);
            float a2 = RSQUARED_2D - dx2 * dx2 - dy2 * dy2;
            if (a2 > 0) {
                value += (a2 * a2) * (a2 * a2) * grad(seed, xsbp, ysbp + PRIME_Y, dx2, dy2);
            }
        }
    }

    return value;
}

float simplex2D(const ULong seed, const float x, const float y)
{
    // Get points for A2* lattice
    float s = SKEW_2D * (x + y);
    float xs = x + s, ys = y + s;
    return noise2UnskewedBase(seed, xs, ys);
}

}   // namespace df