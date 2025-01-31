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
#include "quat.h"
#include "vec3.h"

struct Matrix33
{
    CUDA_CALLABLE Matrix33()
    {
    }
    CUDA_CALLABLE Matrix33(const float* ptr)
    {
        cols[0].x = ptr[0];
        cols[0].y = ptr[1];
        cols[0].z = ptr[2];

        cols[1].x = ptr[3];
        cols[1].y = ptr[4];
        cols[1].z = ptr[5];

        cols[2].x = ptr[6];
        cols[2].y = ptr[7];
        cols[2].z = ptr[8];
    }
    CUDA_CALLABLE Matrix33(const Vec3& c1, const Vec3& c2, const Vec3& c3)
    {
        cols[0] = c1;
        cols[1] = c2;
        cols[2] = c3;
    }

    CUDA_CALLABLE Matrix33(const Quat& q)
    {
        cols[0] = Rotate(q, Vec3(1.0f, 0.0f, 0.0f));
        cols[1] = Rotate(q, Vec3(0.0f, 1.0f, 0.0f));
        cols[2] = Rotate(q, Vec3(0.0f, 0.0f, 1.0f));
    }

    CUDA_CALLABLE float operator()(int i, int j) const
    {
        return static_cast<const float*>(cols[j])[i];
    }
    CUDA_CALLABLE float& operator()(int i, int j)
    {
        return static_cast<float*>(cols[j])[i];
    }

    Vec3 cols[3];

    CUDA_CALLABLE static inline Matrix33 Identity()
    {
        const Matrix33 sIdentity(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
        return sIdentity;
    }
};

CUDA_CALLABLE inline float L2_magnitude(const Matrix33& matrix)
{
    float sum = 0.0;
    for (int i = 0; i < 3; i++)
    {
        sum += matrix.cols[i].x * matrix.cols[i].x;
        sum += matrix.cols[i].y * matrix.cols[i].y;
        sum += matrix.cols[i].z * matrix.cols[i].z;
    }
    return sqrtf(sum);
}


CUDA_CALLABLE inline Matrix33 Diagonalize(const Vec3& v)
{
    Matrix33 r;
    r.cols[0].x = v.x;
    r.cols[1].y = v.y;
    r.cols[2].z = v.z;
    return r;
}

CUDA_CALLABLE inline Matrix33 Multiply(float s, const Matrix33& m)
{
    Matrix33 r = m;
    r.cols[0] *= s;
    r.cols[1] *= s;
    r.cols[2] *= s;
    return r;
}

CUDA_CALLABLE inline Vec3 Multiply(const Matrix33& a, const Vec3& x)
{
    return a.cols[0] * x.x + a.cols[1] * x.y + a.cols[2] * x.z;
}

CUDA_CALLABLE inline Vec3 operator*(const Matrix33& a, const Vec3& x)
{
    return Multiply(a, x);
}

CUDA_CALLABLE inline Matrix33 Multiply(const Matrix33& a, const Matrix33& b)
{
    Matrix33 r;
    r.cols[0] = a * b.cols[0];
    r.cols[1] = a * b.cols[1];
    r.cols[2] = a * b.cols[2];
    return r;
}

CUDA_CALLABLE inline Matrix33 Add(const Matrix33& a, const Matrix33& b)
{
    return Matrix33(a.cols[0] + b.cols[0], a.cols[1] + b.cols[1], a.cols[2] + b.cols[2]);
}

CUDA_CALLABLE inline float Determinant(const Matrix33& m)
{
    return Dot(m.cols[0], Cross(m.cols[1], m.cols[2]));
}

CUDA_CALLABLE inline Matrix33 Transpose(const Matrix33& a)
{
    Matrix33 r;
    for (uint32_t i = 0; i < 3; ++i)
        for (uint32_t j = 0; j < 3; ++j)
            r(i, j) = a(j, i);

    return r;
}

CUDA_CALLABLE inline float Trace(const Matrix33& a)
{
    return a(0, 0) + a(1, 1) + a(2, 2);
}

CUDA_CALLABLE inline Matrix33 Outer(const Vec3& a, const Vec3& b)
{
    return Matrix33(a * b.x, a * b.y, a * b.z);
}

CUDA_CALLABLE inline Matrix33 Inverse(const Matrix33& a, bool& success)
{
    float s = Determinant(a);

    const float eps = 0.0f;

    if (fabsf(s) > eps)
    {
        Matrix33 b;

        b(0, 0) = a(1, 1) * a(2, 2) - a(1, 2) * a(2, 1);
        b(0, 1) = a(0, 2) * a(2, 1) - a(0, 1) * a(2, 2);
        b(0, 2) = a(0, 1) * a(1, 2) - a(0, 2) * a(1, 1);
        b(1, 0) = a(1, 2) * a(2, 0) - a(1, 0) * a(2, 2);
        b(1, 1) = a(0, 0) * a(2, 2) - a(0, 2) * a(2, 0);
        b(1, 2) = a(0, 2) * a(1, 0) - a(0, 0) * a(1, 2);
        b(2, 0) = a(1, 0) * a(2, 1) - a(1, 1) * a(2, 0);
        b(2, 1) = a(0, 1) * a(2, 0) - a(0, 0) * a(2, 1);
        b(2, 2) = a(0, 0) * a(1, 1) - a(0, 1) * a(1, 0);

        success = true;

        return Multiply(1.0f / s, b);
    }
    else
    {
        success = false;
        return Matrix33();
    }
}

CUDA_CALLABLE inline Matrix33 InverseDouble(const Matrix33& a, bool& success)
{

    double m[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            m[i][j] = a(i, j);


    double det = m[0][0] * (m[2][2] * m[1][1] - m[2][1] * m[1][2]) - m[1][0] * (m[2][2] * m[0][1] - m[2][1] * m[0][2]) +
                 m[2][0] * (m[1][2] * m[0][1] - m[1][1] * m[0][2]);

    const double eps = 0.0f;

    if (fabs(det) > eps)
    {
        double b[3][3];

        b[0][0] = m[1][1] * m[2][2] - m[1][2] * m[2][1];
        b[0][1] = m[0][2] * m[2][1] - m[0][1] * m[2][2];
        b[0][2] = m[0][1] * m[1][2] - m[0][2] * m[1][1];
        b[1][0] = m[1][2] * m[2][0] - m[1][0] * m[2][2];
        b[1][1] = m[0][0] * m[2][2] - m[0][2] * m[2][0];
        b[1][2] = m[0][2] * m[1][0] - m[0][0] * m[1][2];
        b[2][0] = m[1][0] * m[2][1] - m[1][1] * m[2][0];
        b[2][1] = m[0][1] * m[2][0] - m[0][0] * m[2][1];
        b[2][2] = m[0][0] * m[1][1] - m[0][1] * m[1][0];

        success = true;

        double invDet = 1.0 / det;

        Matrix33 out;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                out(i, j) = (float)(b[i][j] * invDet);

        return out;
    }
    else
    {
        success = false;

        Matrix33 out;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                out(i, j) = 0.0f;

        return out;
    }
}


CUDA_CALLABLE inline Matrix33 operator*(float s, const Matrix33& a)
{
    return Multiply(s, a);
}
CUDA_CALLABLE inline Matrix33 operator*(const Matrix33& a, float s)
{
    return Multiply(s, a);
}
CUDA_CALLABLE inline Matrix33 operator*(const Matrix33& a, const Matrix33& b)
{
    return Multiply(a, b);
}
CUDA_CALLABLE inline Matrix33 operator+(const Matrix33& a, const Matrix33& b)
{
    return Add(a, b);
}
CUDA_CALLABLE inline Matrix33 operator-(const Matrix33& a, const Matrix33& b)
{
    return Add(a, -1.0f * b);
}
CUDA_CALLABLE inline Matrix33& operator+=(Matrix33& a, const Matrix33& b)
{
    a = a + b;
    return a;
}
CUDA_CALLABLE inline Matrix33& operator-=(Matrix33& a, const Matrix33& b)
{
    a = a - b;
    return a;
}
CUDA_CALLABLE inline Matrix33& operator*=(Matrix33& a, float s)
{
    a.cols[0] *= s;
    a.cols[1] *= s;
    a.cols[2] *= s;
    return a;
}

CUDA_CALLABLE inline Matrix33 Skew(const Vec3& v)
{
    return Matrix33(Vec3(0.0f, v.z, -v.y), Vec3(-v.z, 0.0f, v.x), Vec3(v.y, -v.x, 0.0f));
}

template <typename T>
CUDA_CALLABLE inline XQuat<T>::XQuat(const Matrix33& m)
{
    float tr = m(0, 0) + m(1, 1) + m(2, 2), h;
    if (tr >= 0)
    {
        h = sqrtf(tr + 1);
        w = 0.5f * h;
        h = 0.5f / h;

        x = (m(2, 1) - m(1, 2)) * h;
        y = (m(0, 2) - m(2, 0)) * h;
        z = (m(1, 0) - m(0, 1)) * h;
    }
    else
    {
        unsigned int i = 0;
        if (m(1, 1) > m(0, 0))
            i = 1;
        if (m(2, 2) > m(i, i))
            i = 2;
        switch (i)
        {
        case 0:
            h = sqrtf((m(0, 0) - (m(1, 1) + m(2, 2))) + 1);
            x = 0.5f * h;
            h = 0.5f / h;

            y = (m(0, 1) + m(1, 0)) * h;
            z = (m(2, 0) + m(0, 2)) * h;
            w = (m(2, 1) - m(1, 2)) * h;
            break;
        case 1:
            h = sqrtf((m(1, 1) - (m(2, 2) + m(0, 0))) + 1);
            y = 0.5f * h;
            h = 0.5f / h;

            z = (m(1, 2) + m(2, 1)) * h;
            x = (m(0, 1) + m(1, 0)) * h;
            w = (m(0, 2) - m(2, 0)) * h;
            break;
        case 2:
            h = sqrtf((m(2, 2) - (m(0, 0) + m(1, 1))) + 1);
            z = 0.5f * h;
            h = 0.5f / h;

            x = (m(2, 0) + m(0, 2)) * h;
            y = (m(1, 2) + m(2, 1)) * h;
            w = (m(1, 0) - m(0, 1)) * h;
            break;
        default: // Make compiler happy
            x = y = z = w = 0;
            break;
        }
    }

    *this = Normalize(*this);
}


CUDA_CALLABLE inline void quat2Mat(const XQuat<float>& q, Matrix33& m)
{
    float sqx = q.x * q.x;
    float sqy = q.y * q.y;
    float sqz = q.z * q.z;
    float squ = q.w * q.w;
    float s = 1.f / (sqx + sqy + sqz + squ);

    m(0, 0) = 1.f - 2.f * s * (sqy + sqz);
    m(0, 1) = 2.f * s * (q.x * q.y - q.z * q.w);
    m(0, 2) = 2.f * s * (q.x * q.z + q.y * q.w);
    m(1, 0) = 2.f * s * (q.x * q.y + q.z * q.w);
    m(1, 1) = 1.f - 2.f * s * (sqx + sqz);
    m(1, 2) = 2.f * s * (q.y * q.z - q.x * q.w);
    m(2, 0) = 2.f * s * (q.x * q.z - q.y * q.w);
    m(2, 1) = 2.f * s * (q.y * q.z + q.x * q.w);
    m(2, 2) = 1.f - 2.f * s * (sqx + sqy);
}

// rotation is using new rotation axis
// rotation: Rx(X)*Ry(Y)*Rz(Z)
CUDA_CALLABLE inline void getEulerXYZ(const XQuat<float>& q, float& X, float& Y, float& Z)
{
    Matrix33 rot;
    quat2Mat(q, rot);

    float cy = sqrtf(rot(2, 2) * rot(2, 2) + rot(1, 2) * rot(1, 2));
    if (cy > 1e-6f)
    {
        Z = -atan2(rot(0, 1), rot(0, 0));
        Y = -atan2(-rot(0, 2), cy);
        X = -atan2(rot(1, 2), rot(2, 2));
    }
    else
    {
        Z = -atan2(-rot(1, 0), rot(1, 1));
        Y = -atan2(-rot(0, 2), cy);
        X = 0.0f;
    }
}
