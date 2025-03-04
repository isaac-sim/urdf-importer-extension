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

#include "UrdfTypes.h"

#include <carb/Defines.h>

#include <pybind11/pybind11.h>

#include <stdint.h>
namespace isaacsim
{
namespace asset
{
namespace importer
{
namespace urdf
{
struct ImportConfig
{
    bool mergeFixedJoints = true;
    bool replaceCylindersWithCapsules = false;
    bool convexDecomp = false;
    bool importInertiaTensor = true;
    bool fixBase = true;
    bool selfCollision = false;
    float density = 0.0f; // default density used for objects without mass/inertia, 0 to autocompute
    UrdfJointTargetType defaultDriveType = UrdfJointTargetType::POSITION;
    float defaultDriveStrength = 1e3f;
    float defaultPositionDriveDamping = 1e2f;
    float distanceScale = 1.0f;
    UrdfAxis upVector = { 0.0f, 0.0f, 1.0f };
    bool createPhysicsScene = false;
    bool makeDefaultPrim = false;
    UrdfNormalSubdivisionScheme subdivisionScheme = UrdfNormalSubdivisionScheme::BILINEAR;
    // bool flipVisuals = false;

    bool collisionFromVisuals = false; // Create collision geometry from visual geometry when missing collision.
    bool parseMimic = true;
    bool overrideJointDynamics = false;
};


struct Urdf
{
    CARB_PLUGIN_INTERFACE("isaacsim::asset::importer::urdf::Urdf", 0, 1);

    // Parses a urdf file into a UrdfRobot data structure
    UrdfRobot(CARB_ABI* parseUrdf)(const std::string& assetRoot, const std::string& assetName, ImportConfig& importConfig);
    // Parses a urdf data string into a UrdfRobot data structure
    UrdfRobot(CARB_ABI* parseUrdfString)(const std::string& urdf_str, ImportConfig& importConfig);
    // Imports a UrdfRobot into the stage

    float(CARB_ABI* computeJointNaturalStiffess)(const UrdfRobot& robot, std::string joint, float naturalFrequency);
    // Imports a UrdfRobot into the stage

    std::string(CARB_ABI* importRobot)(const std::string& assetRoot,
                                       const std::string& assetName,
                                       const UrdfRobot& robot,
                                       ImportConfig& importConfig,
                                       const std::string& stage,
                                       const bool getArticulationRoot);

    pybind11::dict(CARB_ABI* getKinematicChain)(const UrdfRobot& robot);
};
}
}
}
}
