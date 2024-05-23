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

#include "../UrdfTypes.h"
#include "../math/core/maths.h"
#include "../parse/UrdfParser.h"
#include "KinematicChain.h"

namespace omni
{
namespace importer
{
namespace urdf
{
Quat indexedRotation(int axis, float s, float c);
Vec3 Diagonalize(const Matrix33& m, Quat& massFrame);
void inertiaToUrdf(const Matrix33& inertia, UrdfInertia& urdfInertia);
void urdfToInertia(const UrdfInertia& urdfInertia, Matrix33& inertia);
void mergeFixedChildLinks(const KinematicChain::Node& parentNode, UrdfRobot& robot);
bool collapseFixedJoints(UrdfRobot& robot);
Vec3 urdfAxisToVec(const UrdfAxis& axis);
std::string resolveXrefPath(const std::string& assetRoot, const std::string& urdfPath, const std::string& xrefpath);
bool IsUsdFile(const std::string& filename);
// Make a path name that is not already used.
std::string GetNewSdfPathString(pxr::UsdStageWeakPtr stage, std::string path, int nameClashNum = -1);
bool addVisualMeshToCollision(UrdfRobot& robot);


}
}
}
