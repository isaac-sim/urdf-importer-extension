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

#define CARB_EXPORTS

// clang-format off
#include "UsdPCH.h"
// clang-format on

#include "import/ImportHelpers.h"
#include "import/UrdfImporter.h"
#include "Urdf.h"

#include <carb/PluginUtils.h>
#include <carb/logging/Log.h>

#include <omni/ext/IExt.h>
#include <omni/kit/IApp.h>
#include <omni/kit/IStageUpdate.h>
#include <pybind11/pybind11.h>

#include <fstream>
#include <memory>

using namespace carb;

const struct carb::PluginImplDesc kPluginImpl = { "omni.importer.urdf", "URDF Utilities", "NVIDIA",
                                                  carb::PluginHotReload::eEnabled, "dev" };

CARB_PLUGIN_IMPL(kPluginImpl, omni::importer::urdf::Urdf)
CARB_PLUGIN_IMPL_DEPS(omni::kit::IApp, carb::logging::ILogging)

namespace
{

omni::importer::urdf::UrdfRobot parseUrdf(const std::string& assetRoot,
                                       const std::string& assetName,
                                       omni::importer::urdf::ImportConfig& importConfig)
{
    omni::importer::urdf::UrdfRobot robot;

    std::string filename = assetRoot + "/" + assetName;
    {


        CARB_LOG_INFO("Trying to import %s", filename.c_str());

        if (parseUrdf(assetRoot, assetName, robot))
        {
        }
        else
        {
            CARB_LOG_ERROR("Failed to parse URDF file '%s'", assetName.c_str());
            return robot;
        }

        if (importConfig.mergeFixedJoints)
        {
            collapseFixedJoints(robot);
        }

        if (importConfig.collisionFromVisuals)
        {
            addVisualMeshToCollision(robot);
        }

        for (auto& joint : robot.joints)
        {
            joint.second.drive.targetType = importConfig.defaultDriveType;
            if (joint.second.drive.targetType == omni::importer::urdf::UrdfJointTargetType::POSITION)
            {
                // set position gain
                if (importConfig.defaultDriveStrength > 0)
                {
                    joint.second.dynamics.stiffness = importConfig.defaultDriveStrength;
                }
                // set velocity gain
                if (importConfig.defaultPositionDriveDamping > 0)
                {
                    joint.second.dynamics.damping = importConfig.defaultPositionDriveDamping;
                }
            }
            else if (joint.second.drive.targetType == omni::importer::urdf::UrdfJointTargetType::VELOCITY)
            {
                // set position gain
                joint.second.dynamics.stiffness = 0.0f;
                // set velocity gain
                if (importConfig.defaultDriveStrength > 0)
                {
                    joint.second.dynamics.damping = importConfig.defaultDriveStrength;
                }
            }
            else if (joint.second.drive.targetType == omni::importer::urdf::UrdfJointTargetType::NONE)
            {
                // set both gains to 0
                joint.second.dynamics.stiffness = 0.0f;
                joint.second.dynamics.damping = 0.0f;
            }
            else
            {
                CARB_LOG_ERROR("Unknown drive target type %d", (int)joint.second.drive.targetType);
            }
        }
    }
    return robot;
}
std::string importRobot(const std::string& assetRoot,
                        const std::string& assetName,
                        const omni::importer::urdf::UrdfRobot& robot,
                        omni::importer::urdf::ImportConfig& importConfig,
                        const std::string& stage_identifier = "")
{

    omni::importer::urdf::UrdfImporter urdfImporter(assetRoot, assetName, importConfig);
    bool save_stage = true;
    pxr::UsdStageRefPtr _stage;
    if (stage_identifier != "" && pxr::UsdStage::IsSupportedFile(stage_identifier))
    {
        _stage = pxr::UsdStage::Open(stage_identifier);
        if (!_stage)
        {
            CARB_LOG_INFO("Creating Stage: %s", stage_identifier.c_str());
            _stage = pxr::UsdStage::CreateNew(stage_identifier);
        }
        else
        {
            for (const auto& p : _stage->GetPrimAtPath(pxr::SdfPath("/")).GetChildren())
            {
                _stage->RemovePrim(p.GetPath());
            }
        }
        importConfig.makeDefaultPrim = true;
        pxr::UsdGeomSetStageUpAxis(_stage, pxr::UsdGeomTokens->z);
    }
    if (!_stage) // If all else fails, import on current stage
    {
        CARB_LOG_INFO("Importing URDF to Current Stage");
        // Get the 'active' USD stage from the USD stage cache.
        const std::vector<pxr::UsdStageRefPtr> allStages = pxr::UsdUtilsStageCache::Get().GetAllStages();
        if (allStages.size() != 1)
        {
            CARB_LOG_ERROR("Cannot determine the 'active' USD stage (%zu stages present in the USD stage cache).", allStages.size());
            return "";
        }

        _stage = allStages[0];
        save_stage = false;
    }
    std::string result = "";
    if (_stage)
    {
        pxr::UsdGeomSetStageMetersPerUnit(_stage, 1.0 / importConfig.distanceScale);
        result = urdfImporter.addToStage(_stage, robot);
        // CARB_LOG_WARN("Import Done, saving");
        if (save_stage)
        {
            // CARB_LOG_WARN("Saving Stage %s", _stage->GetRootLayer()->GetIdentifier().c_str());
            _stage->Save();
        }
    }
    else
    {
        CARB_LOG_ERROR("Stage pointer not valid, could not import urdf to stage");
    }
    return result;
}
}


pybind11::list addLinksAndJoints(omni::importer::urdf::KinematicChain::Node* parentNode)
{
    if (parentNode->parentJointName_ == "")
    {
    }
    pybind11::list temp_list;

    if (!parentNode->childNodes_.empty())
    {
        for (const auto& childNode : parentNode->childNodes_)
        {
            pybind11::dict temp;
            temp["A_joint"] = childNode->parentJointName_;
            temp["A_link"] = parentNode->linkName_;
            temp["B_link"] = childNode->linkName_;
            temp["B_node"] = addLinksAndJoints(childNode.get());
            temp_list.append(temp);
        }
    }
    return temp_list;
}

pybind11::dict getKinematicChain(const omni::importer::urdf::UrdfRobot& robot)
{
    pybind11::dict robotDict;
    omni::importer::urdf::KinematicChain chain;
    if (chain.computeKinematicChain(robot))
    {
        robotDict["A_joint"] = "";
        robotDict["B_link"] = chain.baseNode->linkName_;
        robotDict["B_node"] = addLinksAndJoints(chain.baseNode.get());
    }
    return robotDict;
}

CARB_EXPORT void carbOnPluginStartup()
{
    CARB_LOG_INFO("Startup URDF Extension");
}


CARB_EXPORT void carbOnPluginShutdown()
{
}


void fillInterface(omni::importer::urdf::Urdf& iface)
{
    using namespace omni::importer::urdf;
    memset(&iface, 0, sizeof(iface));
    iface.parseUrdf = parseUrdf;
    iface.importRobot = importRobot;
    iface.getKinematicChain = getKinematicChain;
}
