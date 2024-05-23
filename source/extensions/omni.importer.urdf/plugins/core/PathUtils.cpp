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

#include "PathUtils.h"

#include <carb/logging/Log.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <errno.h>
#include <vector>

#ifdef _WIN32
#    include <direct.h>
#    include <windows.h>
#else
#    include <sys/stat.h>

#    include <dirent.h>
#    include <unistd.h>
#endif

namespace omni
{
namespace importer
{

namespace urdf
{
PathType testPath(const char* path)
{
    if (!path || !*path)
    {
        return PathType::eNone;
    }

#ifdef _WIN32
    DWORD attribs = GetFileAttributesA(path);
    if (attribs == INVALID_FILE_ATTRIBUTES)
    {
        return PathType::eNone;
    }
    else if (attribs & FILE_ATTRIBUTE_DIRECTORY)
    {
        return PathType::eDirectory;
    }
    else
    {
        // hmmm
        return PathType::eFile;
    }
#else
    struct stat s;
    if (stat(path, &s) == -1)
    {
        return PathType::eNone;
    }
    else if (S_ISREG(s.st_mode))
    {
        return PathType::eFile;
    }
    else if (S_ISDIR(s.st_mode))
    {
        return PathType::eDirectory;
    }
    else
    {
        return PathType::eOther;
    }
#endif
}

bool isAbsolutePath(const char* path)
{
    if (!path || !*path)
    {
        return false;
    }

#ifdef _WIN32
    if (path[0] == '\\' || path[0] == '/')
    {
        return true;
    }
    else if (std::isalpha(path[0]) && path[1] == ':')
    {
        return true;
    }
    else
    {
        return false;
    }
#else
    return path[0] == '/';
#endif
}

std::string pathJoin(const std::string& path1, const std::string& path2)
{
    if (path1.empty())
    {
        return path2;
    }
    else
    {
        auto last = path1.rbegin();
#ifdef _WIN32
        if (*last != '/' && *last != '\\')
#else
        if (*last != '/')
#endif
        {
            return path1 + '/' + path2;
        }
        else
        {
            return path1 + path2;
        }
    }
}

std::string getCwd()
{
    std::vector<char> buf(4096);
#ifdef _WIN32
    if (!_getcwd(buf.data(), int(buf.size())))
#else
    if (!getcwd(buf.data(), size_t(buf.size())))
#endif
    {
        return std::string();
    }
    return std::string(buf.data());
}

static int sysmkdir(const char* path)
{
#ifdef _WIN32
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

static std::vector<std::string> tokenizePath(const char* path_)
{
    std::vector<std::string> components;

    if (!path_ || !*path_)
    {
        return components;
    }

    std::string path(path_);

    size_t start = 0;
    bool done = false;
    while (!done)
    {
#ifdef _WIN32
        size_t end = path.find_first_of("/\\", start);
#else
        size_t end = path.find_first_of('/', start);
#endif
        if (end == std::string::npos)
        {
            end = path.length();
            done = true;
        }
        if (end - start > 0)
        {
            components.push_back(path.substr(start, end - start));
        }
        start = end + 1;
    }

    return components;
}

bool makeDirectory(const char* path)
{
    std::vector<std::string> components = tokenizePath(path);
    if (components.empty())
    {
        return false;
    }

    std::string pathSoFar;
#ifndef _WIN32
    // on Unixes, need to start with leading slash if absolute path
    if (isAbsolutePath(path))
    {
        pathSoFar = "/";
    }
#endif
    for (unsigned i = 0; i < components.size(); i++)
    {
        if (i > 0)
        {
            pathSoFar += '/';
        }
        pathSoFar += components[i];
        if (sysmkdir(pathSoFar.c_str()) != 0 && errno != EEXIST)
        {
            fprintf(stderr, "*** Failed to create directory '%s'\n", pathSoFar.c_str());
            return false;
        }
        // printf("Creating '%s'\n", pathSoFar.c_str());
    }

    return true;
}

std::string getPathStem(const char* path)
{
    if (!path || !*path)
    {
        return "";
    }

    const char* p = strrchr(path, '/');
#ifdef _WIN32
    const char* q = strrchr(path, '\\');
    if (q > p)
    {
        p = q;
    }
#endif

    const char* fnameStart = p ? p + 1 : path;
    const char* ext = strrchr(fnameStart, '.');
    if (ext)
    {
        return std::string(fnameStart, ext);
    }
    else
    {
        return fnameStart;
    }
}

static void getFileListRecursiveRec(const std::string& dir, std::vector<std::string>& flist)
{
#if _WIN32
    WIN32_FIND_DATAA fdata;
    memset(&fdata, 0, sizeof(fdata));

    std::string pattern = dir + "\\*";

    HANDLE handle = FindFirstFileA(pattern.c_str(), &fdata);
    while (handle != INVALID_HANDLE_VALUE)
    {
        if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
        {
            if (strcmp(fdata.cFileName, ".") != 0 && strcmp(fdata.cFileName, "..") != 0)
            {
                getFileListRecursiveRec(dir + '\\' + fdata.cFileName, flist);
            }
        }
        else
        {
            flist.push_back(dir + '\\' + fdata.cFileName);
        }

        if (FindNextFileA(handle, &fdata) == FALSE)
        {
            break;
        }
    }
    FindClose(handle);
#else
    DIR* d = opendir(dir.c_str());
    if (d)
    {
        struct dirent* dent;
        while ((dent = readdir(d)) != nullptr)
        {
            if (dent->d_type == DT_DIR)
            {
                if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0)
                {
                    getFileListRecursiveRec(dir + '/' + dent->d_name, flist);
                }
            }
            else if (dent->d_type == DT_REG)
            {
                flist.push_back(dir + '/' + dent->d_name);
            }
        }
        closedir(d);
    }
#endif
}

std::vector<std::string> getFileListRecursive(const std::string& dir, bool sorted)
{
    std::vector<std::string> flist;

    getFileListRecursiveRec(dir, flist);

    if (sorted)
    {
        std::sort(flist.begin(), flist.end());
    }

    return flist;
}

std::string makeValidUSDIdentifier(const std::string& name)
{
    auto validName = pxr::TfMakeValidIdentifier(name);
    if (validName[0] == '_')
    {
        validName = "a" + validName;
    }
    if (pxr::TfIsValidIdentifier(name) == false)
    {
        CARB_LOG_WARN("The path %s is not a valid usd path, modifying to %s", name.c_str(), validName.c_str());
    }

    return validName;
}


std::string getParent(const std::string& filePath)
{
    size_t found = filePath.find_last_of("/\\");
    if (found != std::string::npos)
    {
        return filePath.substr(0, found);
    }
    return "";
}

bool createSymbolicLink(const std::string& target, const std::string& link)
{
#ifdef _WIN32
    // Windows implementation
    // Convert std::string to LPCWSTR (wide-character string) for Windows API function
    LPCWSTR lpSourcePath = reinterpret_cast<LPCWSTR>(target.c_str());
    LPCWSTR lpLinkPath = reinterpret_cast<LPCWSTR>(link.c_str());
    if (!CreateSymbolicLink(lpLinkPath, lpSourcePath, 0))
    {
        CARB_LOG_ERROR("Failed to create symbolic link: %s -> %s.\n Error Code: %lu", target.c_str(), link.c_str(),
                       GetLastError());
        return false;
    }
#elif __linux__
    // Linux implementation
    if (symlink(target.c_str(), link.c_str()) != 0)
    {
        CARB_LOG_ERROR("Failed to create symbolic link: %s -> %s", target.c_str(), link.c_str());
        return false;
    }
#else
#    error "Unsupported platform"
#endif
    return true;
}

// Function to convert a string to lowercase
std::string toLowercase(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool hasExtension(const std::string& filename, const std::string& extension)
{
    // Get the position of the last dot in the filename
    size_t dotPosition = filename.find_last_of('.');

    // If there's no dot, it means there's no extension
    if (dotPosition == std::string::npos)
    {
        return false;
    }

    // Get the substring after the dot (the extension) and convert to lowercase
    std::string fileExtension = toLowercase(filename.substr(dotPosition + 1));

    // Convert the extension to check to lowercase
    std::string extensionToCheckLowercase = toLowercase(extension);

    // Compare the lowercase extension with the lowercase extension to check
    return fileExtension == extensionToCheckLowercase;
}

}
}
}
