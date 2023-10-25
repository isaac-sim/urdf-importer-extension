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
// clang-format off
#include "../UsdPCH.h"
// clang-format on


#include "assimp/scene.h"

#include <string>


namespace omni
{
namespace importer
{
namespace urdf
{


pxr::SdfPath SimpleImport(pxr::UsdStageRefPtr usdStage,
                          std::string path,
                          const aiScene* mScene,
                          const std::string mesh_path,
                          std::map<pxr::TfToken, std::string>& materialsList,
                          const bool loadMaterials = true,
                          const bool flipVisuals = false,
                          const char* subdvisionScheme = "none",
                          const bool instanceable = false);


}
}
}
