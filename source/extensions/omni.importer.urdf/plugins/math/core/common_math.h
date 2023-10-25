// SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "core.h"
#include "types.h"

#include <cassert>
#include <cmath>
#include <float.h>
#include <string.h>

#ifdef __CUDACC__
#    define CUDA_CALLABLE __host__ __device__
#else
#    define CUDA_CALLABLE
#endif

#define kPi 3.141592653589f
const float k2Pi = 2.0f * kPi;
const float kInvPi = 1.0f / kPi;
const float kInv2Pi = 0.5f / kPi;
const float kDegToRad = kPi / 180.0f;
const float kRadToDeg = 180.0f / kPi;

CUDA_CALLABLE inline float DegToRad(float t)
{
    return t * kDegToRad;
}

CUDA_CALLABLE inline float RadToDeg(float t)
{
    return t * kRadToDeg;
}

CUDA_CALLABLE inline float Sin(float theta)
{
    return sinf(theta);
}

CUDA_CALLABLE inline float Cos(float theta)
{
    return cosf(theta);
}

CUDA_CALLABLE inline void SinCos(float theta, float& s, float& c)
{
    // no optimizations yet
    s = sinf(theta);
    c = cosf(theta);
}

CUDA_CALLABLE inline float Tan(float theta)
{
    return tanf(theta);
}

CUDA_CALLABLE inline float Sqrt(float x)
{
    return sqrtf(x);
}

CUDA_CALLABLE inline double Sqrt(double x)
{
    return sqrt(x);
}

CUDA_CALLABLE inline float ASin(float theta)
{
    return asinf(theta);
}

CUDA_CALLABLE inline float ACos(float theta)
{
    return acosf(theta);
}

CUDA_CALLABLE inline float ATan(float theta)
{
    return atanf(theta);
}

CUDA_CALLABLE inline float ATan2(float x, float y)
{
    return atan2f(x, y);
}

CUDA_CALLABLE inline float Abs(float x)
{
    return fabsf(x);
}

CUDA_CALLABLE inline float Pow(float b, float e)
{
    return powf(b, e);
}

CUDA_CALLABLE inline float Sgn(float x)
{
    return (x < 0.0f ? -1.0f : 1.0f);
}

CUDA_CALLABLE inline float Sign(float x)
{
    return x < 0.0f ? -1.0f : 1.0f;
}

CUDA_CALLABLE inline double Sign(double x)
{
    return x < 0.0f ? -1.0f : 1.0f;
}

CUDA_CALLABLE inline float Mod(float x, float y)
{
    return fmod(x, y);
}

template <typename T>
CUDA_CALLABLE inline T Min(T a, T b)
{
    return a < b ? a : b;
}

template <typename T>
CUDA_CALLABLE inline T Max(T a, T b)
{
    return a > b ? a : b;
}

template <typename T>
CUDA_CALLABLE inline void Swap(T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

template <typename T>
CUDA_CALLABLE inline T Clamp(T a, T low, T high)
{
    if (low > high)
        Swap(low, high);

    return Max(low, Min(a, high));
}

template <typename V, typename T>
CUDA_CALLABLE inline V Lerp(const V& start, const V& end, const T& t)
{
    return start + (end - start) * t;
}

CUDA_CALLABLE inline float InvSqrt(float x)
{
    return 1.0f / sqrtf(x);
}

// round towards +infinity
CUDA_CALLABLE inline int Round(float f)
{
    return int(f + 0.5f);
}

template <typename T>
CUDA_CALLABLE T Normalize(const T& v)
{
    T a(v);
    a /= Length(v);
    return a;
}

template <typename T>
CUDA_CALLABLE inline typename T::value_type LengthSq(const T v)
{
    return Dot(v, v);
}

template <typename T>
CUDA_CALLABLE inline typename T::value_type Length(const T& v)
{
    typename T::value_type lSq = LengthSq(v);
    if (lSq)
        return Sqrt(LengthSq(v));
    else
        return 0.0f;
}

// this is mainly a helper function used by script
template <typename T>
CUDA_CALLABLE inline typename T::value_type Distance(const T& v1, const T& v2)
{
    return Length(v1 - v2);
}

template <typename T>
CUDA_CALLABLE inline T SafeNormalize(const T& v, const T& fallback = T())
{
    float l = LengthSq(v);
    if (l > 0.0f)
    {
        return v * InvSqrt(l);
    }
    else
        return fallback;
}

template <typename T>
CUDA_CALLABLE inline T Sqr(T x)
{
    return x * x;
}

template <typename T>
CUDA_CALLABLE inline T Cube(T x)
{
    return x * x * x;
}
