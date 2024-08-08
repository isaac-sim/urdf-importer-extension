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

// clang-format off
#include "../UsdPCH.h"
// clang-format on

#include <carb/logging/Log.h>

#include <OmniClient.h>
#include <string>

// Platform-specific headers
#ifdef _WIN32
#    include <filesystem>
#else
#    include <sys/stat.h>
#endif

inline bool isFile(const std::string& path)
{
#ifdef _WIN32
    namespace fs = std::filesystem;

    return fs::is_regular_file(path); // Checks if the path is a regular file

#else
    // POSIX/Unix-like systems implementation
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0)
    {
        // Path does not exist
        return false;
    }
    // Check if it's a regular file
    return S_ISREG(pathStat.st_mode);
#endif
}


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


inline std::string resolve_absolute(std::string parent, std::string relative)
{
    size_t bufferSize = parent.size() + relative.size();
    std::unique_ptr<char[]> stringBuffer = std::unique_ptr<char[]>(new char[bufferSize]);
    std::string combined_url = normalizeUrl((parent + "/" + relative).c_str());
    return combined_url;
}


// Helper function to split a path into components
inline std::vector<std::string> split_path(const std::string& path)
{
    std::vector<std::string> components;
    size_t start = 0;
    size_t end = path.find('/');
    while (end != std::string::npos)
    {
        components.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find('/', start);
    }
    components.push_back(path.substr(start));
    return components;
}

/**
 * Calculates the relative path of target relative to base.
 *
 * @param base The base path.
 * @param target The target path.
 * @return The relative path of target relative to base.
 * @throws std::invalid_argument If base or target is not a valid path.
 */
inline std::string resolve_relative(const std::string& base, const std::string& target)
{

    if (!isFile(target))
    {
        return target;
    }
    // Normalize both paths to use '/' as the separator
    std::string base_normalized = normalizeUrl(base.c_str());
    std::string target_normalized = normalizeUrl(target.c_str());

    // Split both paths into components
    std::vector<std::string> base_components = split_path(base_normalized);
    std::vector<std::string> target_components = split_path(target_normalized);

    if (isFile(base_normalized))
    {
        base_components.pop_back();
    }

    // Find the common prefix
    size_t common_prefix_length = 0;
    while (common_prefix_length < base_components.size() && common_prefix_length < target_components.size() &&
           base_components[common_prefix_length] == target_components[common_prefix_length])
    {
        common_prefix_length++;
    }

    // Calculate the relative path
    std::string relative_path;
    for (size_t i = common_prefix_length; i < base_components.size(); i++)
    {
        relative_path += "../";
    }
    for (size_t i = common_prefix_length; i < target_components.size(); i++)
    {
        relative_path += target_components[i] + "/";
    }

    // Remove the trailing '/'
    if (!relative_path.empty() && relative_path.back() == '/')
    {
        relative_path.pop_back();
    }

    return relative_path;
}

/**
 * Calculates the final path by resolving the relative path and removing any unnecessary components.
 *
 * @param path The input path string.
 * @return The computed final path.
 * @throws std::invalid_argument If the path is not a valid path.
 */
inline std::string resolve_path(const std::string& path)
{
    // Normalize the path to use '/' as the separator
    std::string normalized_path = normalizeUrl(path.c_str());

    // Split the path into components
    std::vector<std::string> components = split_path(normalized_path);

    // Resolve the relative path
    std::vector<std::string> resolved_components;
    for (const std::string& component : components)
    {
        if (component == "..")
        {
            if (!resolved_components.empty())
            {
                resolved_components.pop_back();
            }
        }
        else if (component != "." && component != "")
        {
            resolved_components.push_back(component);
        }
    }

    // Reconstruct the final path
    std::string final_path;
    if (path[0] == '/')
    {
        final_path += "/";
    }
    for (const std::string& component : resolved_components)
    {
        final_path += component + "/";
    }

    // Remove the trailing '/'
    if (!final_path.empty() && final_path.back() == '/')
    {
        final_path.pop_back();
    }

    return final_path;
}
