// SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "common_math.h"

#if defined(_WIN32) && !defined(__CUDACC__)
#    if defined(_DEBUG)

#        define VEC2_VALIDATE()                                                                                        \
            {                                                                                                          \
                assert(_finite(x));                                                                                    \
                assert(!_isnan(x));                                                                                    \
                                                                                                                       \
                assert(_finite(y));                                                                                    \
                assert(!_isnan(y));                                                                                    \
            }
#    else

#        define VEC2_VALIDATE()                                                                                        \
            {                                                                                                          \
                assert(isfinite(x));                                                                                   \
                assert(isfinite(y));                                                                                   \
            }

#    endif // _WIN32

#else
#    define VEC2_VALIDATE()
#endif

#ifdef _DEBUG
#    define FLOAT_VALIDATE(f)                                                                                          \
        {                                                                                                              \
            assert(_finite(f));                                                                                        \
            assert(!_isnan(f));                                                                                        \
        }
#else
#    define FLOAT_VALIDATE(f)
#endif


// vec2
template <typename T>
class XVector2
{
public:
    typedef T value_type;

    CUDA_CALLABLE XVector2() : x(0.0f), y(0.0f)
    {
        VEC2_VALIDATE();
    }
    CUDA_CALLABLE XVector2(T _x) : x(_x), y(_x)
    {
        VEC2_VALIDATE();
    }
    CUDA_CALLABLE XVector2(T _x, T _y) : x(_x), y(_y)
    {
        VEC2_VALIDATE();
    }
    CUDA_CALLABLE XVector2(const T* p) : x(p[0]), y(p[1])
    {
    }

    template <typename U>
    CUDA_CALLABLE explicit XVector2(const XVector2<U>& v) : x(v.x), y(v.y)
    {
    }

    CUDA_CALLABLE operator T*()
    {
        return &x;
    }
    CUDA_CALLABLE operator const T*() const
    {
        return &x;
    };

    CUDA_CALLABLE void Set(T x_, T y_)
    {
        VEC2_VALIDATE();
        x = x_;
        y = y_;
    }

    CUDA_CALLABLE XVector2<T> operator*(T scale) const
    {
        XVector2<T> r(*this);
        r *= scale;
        VEC2_VALIDATE();
        return r;
    }
    CUDA_CALLABLE XVector2<T> operator/(T scale) const
    {
        XVector2<T> r(*this);
        r /= scale;
        VEC2_VALIDATE();
        return r;
    }
    CUDA_CALLABLE XVector2<T> operator+(const XVector2<T>& v) const
    {
        XVector2<T> r(*this);
        r += v;
        VEC2_VALIDATE();
        return r;
    }
    CUDA_CALLABLE XVector2<T> operator-(const XVector2<T>& v) const
    {
        XVector2<T> r(*this);
        r -= v;
        VEC2_VALIDATE();
        return r;
    }

    CUDA_CALLABLE XVector2<T>& operator*=(T scale)
    {
        x *= scale;
        y *= scale;
        VEC2_VALIDATE();
        return *this;
    }
    CUDA_CALLABLE XVector2<T>& operator/=(T scale)
    {
        T s(1.0f / scale);
        x *= s;
        y *= s;
        VEC2_VALIDATE();
        return *this;
    }
    CUDA_CALLABLE XVector2<T>& operator+=(const XVector2<T>& v)
    {
        x += v.x;
        y += v.y;
        VEC2_VALIDATE();
        return *this;
    }
    CUDA_CALLABLE XVector2<T>& operator-=(const XVector2<T>& v)
    {
        x -= v.x;
        y -= v.y;
        VEC2_VALIDATE();
        return *this;
    }

    CUDA_CALLABLE XVector2<T>& operator*=(const XVector2<T>& scale)
    {
        x *= scale.x;
        y *= scale.y;
        VEC2_VALIDATE();
        return *this;
    }

    // negate
    CUDA_CALLABLE XVector2<T> operator-() const
    {
        VEC2_VALIDATE();
        return XVector2<T>(-x, -y);
    }

    T x;
    T y;
};

typedef XVector2<float> Vec2;
typedef XVector2<float> Vector2;

// lhs scalar scale
template <typename T>
CUDA_CALLABLE XVector2<T> operator*(T lhs, const XVector2<T>& rhs)
{
    XVector2<T> r(rhs);
    r *= lhs;
    return r;
}

template <typename T>
CUDA_CALLABLE XVector2<T> operator*(const XVector2<T>& lhs, const XVector2<T>& rhs)
{
    XVector2<T> r(lhs);
    r *= rhs;
    return r;
}

template <typename T>
CUDA_CALLABLE bool operator==(const XVector2<T>& lhs, const XVector2<T>& rhs)
{
    return (lhs.x == rhs.x && lhs.y == rhs.y);
}


template <typename T>
CUDA_CALLABLE T Dot(const XVector2<T>& v1, const XVector2<T>& v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

// returns the ccw perpendicular vector
template <typename T>
CUDA_CALLABLE XVector2<T> PerpCCW(const XVector2<T>& v)
{
    return XVector2<T>(-v.y, v.x);
}

template <typename T>
CUDA_CALLABLE XVector2<T> PerpCW(const XVector2<T>& v)
{
    return XVector2<T>(v.y, -v.x);
}

// component wise min max functions
template <typename T>
CUDA_CALLABLE XVector2<T> Max(const XVector2<T>& a, const XVector2<T>& b)
{
    return XVector2<T>(Max(a.x, b.x), Max(a.y, b.y));
}

template <typename T>
CUDA_CALLABLE XVector2<T> Min(const XVector2<T>& a, const XVector2<T>& b)
{
    return XVector2<T>(Min(a.x, b.x), Min(a.y, b.y));
}

// 2d cross product, treat as if a and b are in the xy plane and return magnitude of z
template <typename T>
CUDA_CALLABLE T Cross(const XVector2<T>& a, const XVector2<T>& b)
{
    return (a.x * b.y - a.y * b.x);
}
