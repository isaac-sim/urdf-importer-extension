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

#include "vec4.h"

#include <ostream>

class Point3
{
public:
    CUDA_CALLABLE Point3() : x(0), y(0), z(0)
    {
    }
    CUDA_CALLABLE Point3(float a) : x(a), y(a), z(a)
    {
    }
    CUDA_CALLABLE Point3(const float* p) : x(p[0]), y(p[1]), z(p[2])
    {
    }
    CUDA_CALLABLE Point3(float x_, float y_, float z_) : x(x_), y(y_), z(z_)
    {
        Validate();
    }

    CUDA_CALLABLE explicit Point3(const Vec3& v) : x(v.x), y(v.y), z(v.z)
    {
    }

    CUDA_CALLABLE operator float*()
    {
        return &x;
    }
    CUDA_CALLABLE operator const float*() const
    {
        return &x;
    };
    CUDA_CALLABLE operator Vec4() const
    {
        return Vec4(x, y, z, 1.0f);
    }

    CUDA_CALLABLE void Set(float x_, float y_, float z_)
    {
        Validate();
        x = x_;
        y = y_;
        z = z_;
    }

    CUDA_CALLABLE Point3 operator*(float scale) const
    {
        Point3 r(*this);
        r *= scale;
        Validate();
        return r;
    }
    CUDA_CALLABLE Point3 operator/(float scale) const
    {
        Point3 r(*this);
        r /= scale;
        Validate();
        return r;
    }
    CUDA_CALLABLE Point3 operator+(const Vec3& v) const
    {
        Point3 r(*this);
        r += v;
        Validate();
        return r;
    }
    CUDA_CALLABLE Point3 operator-(const Vec3& v) const
    {
        Point3 r(*this);
        r -= v;
        Validate();
        return r;
    }

    CUDA_CALLABLE Point3& operator*=(float scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        Validate();
        return *this;
    }
    CUDA_CALLABLE Point3& operator/=(float scale)
    {
        float s(1.0f / scale);
        x *= s;
        y *= s;
        z *= s;
        Validate();
        return *this;
    }
    CUDA_CALLABLE Point3& operator+=(const Vec3& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        Validate();
        return *this;
    }
    CUDA_CALLABLE Point3& operator-=(const Vec3& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        Validate();
        return *this;
    }

    CUDA_CALLABLE Point3& operator=(const Vec3& v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    CUDA_CALLABLE bool operator!=(const Point3& v) const
    {
        return (x != v.x || y != v.y || z != v.z);
    }

    // negate
    CUDA_CALLABLE Point3 operator-() const
    {
        Validate();
        return Point3(-x, -y, -z);
    }

    float x, y, z;

    CUDA_CALLABLE void Validate() const
    {
    }
};

// lhs scalar scale
CUDA_CALLABLE inline Point3 operator*(float lhs, const Point3& rhs)
{
    Point3 r(rhs);
    r *= lhs;
    return r;
}

CUDA_CALLABLE inline Vec3 operator-(const Point3& lhs, const Point3& rhs)
{
    return Vec3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

CUDA_CALLABLE inline Point3 operator+(const Point3& lhs, const Point3& rhs)
{
    return Point3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

CUDA_CALLABLE inline bool operator==(const Point3& lhs, const Point3& rhs)
{
    return (lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

// component wise min max functions
CUDA_CALLABLE inline Point3 Max(const Point3& a, const Point3& b)
{
    return Point3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z));
}

CUDA_CALLABLE inline Point3 Min(const Point3& a, const Point3& b)
{
    return Point3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z));
}
