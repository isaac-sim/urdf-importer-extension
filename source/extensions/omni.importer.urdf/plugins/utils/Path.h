// SPDX-FileCopyrightText: Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

// clang-format off
#include "../UsdPCH.h"
// clang-format on

#include <OmniClient.h>
#include <string>

namespace omni
{
namespace importer
{
namespace utils
{
namespace path
{

inline std::string normalizeUrl(const char* url)
{
    std::string ret;
    char stringBuffer[1024];
    std::unique_ptr<char[]> stringBufferHeap;
    size_t bufferSize = sizeof(stringBuffer);
    const char* normalizedUrl = omniClientNormalizeUrl(url, stringBuffer, &bufferSize);
    if (!normalizedUrl)
    {
        stringBufferHeap = std::unique_ptr<char[]>(new char[bufferSize]);
        normalizedUrl = omniClientNormalizeUrl(url, stringBufferHeap.get(), &bufferSize);
        if (!normalizedUrl)
        {
            normalizedUrl = "";
            CARB_LOG_ERROR("Cannot normalize %s", url);
        }
    }

    ret = normalizedUrl;
    for (auto& c : ret)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return ret;
}


std::string resolve_absolute(std::string parent, std::string relative)
{
    size_t bufferSize = parent.size() + relative.size();
    std::unique_ptr<char[]> stringBuffer = std::unique_ptr<char[]>(new char[bufferSize]);
    std::string combined_url = normalizeUrl((parent + "/" + relative).c_str());
    return combined_url;
}

}
}
}
}
