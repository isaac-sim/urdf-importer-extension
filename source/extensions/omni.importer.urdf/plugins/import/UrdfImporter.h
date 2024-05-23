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

#include "../Urdf.h"
#include "../UrdfTypes.h"
#include "../math/core/maths.h"
#include "../parse/UrdfParser.h"
#include "KinematicChain.h"
#include "MeshImporter.h"

#include <carb/logging/Log.h>

#include <pxr/usd/usdPhysics/articulationRootAPI.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/driveAPI.h>
#include <pxr/usd/usdPhysics/fixedJoint.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/prismaticJoint.h>
#include <pxr/usd/usdPhysics/revoluteJoint.h>
#include <pxr/usd/usdPhysics/scene.h>
#include <pxr/usd/usdPhysics/sphericalJoint.h>

namespace omni
{
namespace importer
{
namespace urdf
{

class UrdfImporter
{
private:
    std::string assetRoot_;
    std::string urdfPath_;
    const ImportConfig config;
    std::map<std::string, std::string> matPrimPaths;
    std::map<pxr::TfToken, std::string> materialsList;

public:
    UrdfImporter(const std::string& assetRoot, const std::string& urdfPath, const ImportConfig& options)
        : assetRoot_(assetRoot), urdfPath_(urdfPath), config(options)
    {
    }

    // Creates and populates a GymAsset
    UrdfRobot createAsset();

    std::string addToStage(pxr::UsdStageWeakPtr stage, const UrdfRobot& robot, const bool getArticulationRoot);


private:
    void buildInstanceableStage(pxr::UsdStageRefPtr stage,
                                const KinematicChain::Node* parentNode,
                                const std::string& robotBasePath,
                                const UrdfRobot& urdfRobot);
    void addInstanceableMeshes(pxr::UsdStageRefPtr stage,
                               const UrdfLink& link,
                               const std::string& robotBasePath,
                               const UrdfRobot& robot);
    void addRigidBody(pxr::UsdStageWeakPtr stage,
                      const UrdfLink& link,
                      const Transform& poseBodyToWorld,
                      pxr::UsdGeomXform robotPrim,
                      const UrdfRobot& robot);
    void addJoint(pxr::UsdStageWeakPtr stage,
                  pxr::UsdGeomXform robotPrim,
                  const UrdfJoint& joint,
                  const Transform& poseJointToParentBody);
    void addLinksAndJoints(pxr::UsdStageWeakPtr stage,
                           const Transform& poseParentToWorld,
                           const KinematicChain::Node* parentNode,
                           const UrdfRobot& robot,
                           pxr::UsdGeomXform robotPrim);
    void addMaterials(pxr::UsdStageWeakPtr stage, const UrdfRobot& robot, const pxr::SdfPath& prefixPath);
    pxr::UsdShadeMaterial addMaterial(pxr::UsdStageWeakPtr stage,
                                      const std::pair<std::string, UrdfMaterial>& mat,
                                      const pxr::SdfPath& prefixPath);
};
}
}
}
