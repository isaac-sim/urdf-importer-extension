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

#include <string>
#include <vector>

namespace isaacsim
{
namespace asset
{
namespace importer
{
namespace urdf
{
enum class PathType
{
    eNone, // path does not exist
    eFile, // path is a regular file
    eDirectory, // path is a directory
    eOther, // path is something else
};

PathType testPath(const char* path);

bool isAbsolutePath(const char* path);

bool makeDirectory(const char* path);

std::string pathJoin(const std::string& path1, const std::string& path2);

std::string getCwd();

// Convert a url path to a valid Sdf path
std::string convertToSdfPath(const std::string& Path, bool absolute = true);

// returns filename without extension (e.g. "foo/bar/bingo.txt" -> "bingo")
std::string getPathStem(const char* path);

std::vector<std::string> getFileListRecursive(const std::string& dir, bool sorted = true);

std::string makeValidUSDIdentifier(const std::string& name);

std::string getParent(const std::string& filePath);

bool createSymbolicLink(const std::string& target, const std::string& link);

std::string toLowercase(const std::string& str);

bool hasExtension(const std::string& filename, const std::string& extension);

}
}
}
}
