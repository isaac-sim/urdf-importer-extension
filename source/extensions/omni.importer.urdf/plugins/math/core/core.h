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

#define ENABLE_VERBOSE_OUTPUT 0
#define ENABLE_APIC_CAPTURE 0
#define ENABLE_PERFALYZE_CAPTURE 0

#if ENABLE_VERBOSE_OUTPUT
#    define VERBOSE(a) a##;
#else
#    define VERBOSE(a)
#endif

//#define Super __super

// basically just a collection of macros and types
#ifndef UNUSED
#    define UNUSED(x) (void)x;
#endif

#define NOMINMAX

#if !PLATFORM_OPENCL
#    include <cassert>
#endif

#include "types.h"

#if !PLATFORM_SPU && !PLATFORM_OPENCL
#    include <algorithm>
#    include <fstream>
#    include <functional>
#    include <iostream>
#    include <string>
#endif

#include <string.h>


// disable some warnings
#if _WIN32
#    pragma warning(disable : 4996) // secure io
#    pragma warning(disable : 4100) // unreferenced param
#    pragma warning(disable : 4324) // structure was padded due to __declspec(align())
#endif

// alignment helpers
#define DEFAULT_ALIGNMENT 16

#if PLATFORM_LINUX
#    define ALIGN_N(x)
#    define ENDALIGN_N(x) __attribute__((aligned(x)))
#else
#    define ALIGN_N(x) __declspec(align(x))
#    define END_ALIGN_N(x)
#endif

#define ALIGN ALIGN_N(DEFAULT_ALIGNMENT)
#define END_ALIGN END_ALIGN_N(DEFAULT_ALIGNMENT)

inline bool IsPowerOfTwo(int n)
{
    return (n & (n - 1)) == 0;
}

// align a ptr to a power of tow
template <typename T>
inline T* AlignPtr(T* p, uint32_t alignment)
{
    assert(IsPowerOfTwo(alignment));

    // cast to safe ptr type
    uintptr_t up = reinterpret_cast<uintptr_t>(p);
    return (T*)((up + (alignment - 1)) & ~(alignment - 1));
}

// align an unsigned value to a power of two
inline uint32_t Align(uint32_t val, uint32_t alignment)
{
    assert(IsPowerOfTwo(alignment));

    return (val + (alignment - 1)) & ~(alignment - 1);
}

inline bool IsAligned(void* p, uint32_t alignment)
{
    return (((uintptr_t)p) & (alignment - 1)) == 0;
}

template <typename To, typename From>
To UnionCast(From in)
{
    union
    {
        To t;
        From f;
    };

    f = in;

    return t;
}

// Endian helpers
template <typename T>
T ByteSwap(const T& val)
{
    T copy = val;
    uint8_t* p = reinterpret_cast<uint8_t*>(&copy);

    std::reverse(p, p + sizeof(T));

    return copy;
}

#ifndef LITTLE_ENDIAN
#    define LITTLE_ENDIAN WIN32
#endif

#ifndef BIG_ENDIAN
#    define BIG_ENDIAN PLATFORM_PS3 || PLATFORM_SPU
#endif

#if BIG_ENDIAN
#    define ToLittleEndian(x) ByteSwap(x)
#else
#    define ToLittleEndian(x) x
#endif

//#include "platform.h"

//#define sizeof_array(x) (sizeof(x)/sizeof(*x))
template <typename T, size_t N>
size_t sizeof_array(const T (&)[N])
{
    return N;
}

// unary_function depricated in c++11
// functor designed for use in the stl
// template <typename T>
// class free_ptr : public std::unary_function<T*, void>
//{
// public:
//    void operator()(const T* ptr)
//    {
//        delete ptr;
//    }
//};


// given the path of one file it strips the filename and appends the relative path onto it
inline void MakeRelativePath(const char* filePath, const char* fileRelativePath, char* fullPath)
{
    // get base path of file
    const char* lastSlash = nullptr;

    if (!lastSlash)
        lastSlash = strrchr(filePath, '\\');
    if (!lastSlash)
        lastSlash = strrchr(filePath, '/');

    int baseLength = 0;

    if (lastSlash)
    {
        baseLength = int(lastSlash - filePath) + 1;

        // copy base path (including slash to relative path)
        memcpy(fullPath, filePath, baseLength);
    }

    // if (fileRelativePath[0] == '.')
    //++fileRelativePath;
    if (fileRelativePath[0] == '\\' || fileRelativePath[0] == '/')
        ++fileRelativePath;

    // append mesh filename
    strcpy(fullPath + baseLength, fileRelativePath);
}
