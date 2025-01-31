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

#define CARB_EXPORTS

// clang-format off
#include "UsdPCH.h"
// clang-format on

#include "Urdf.h"

#include "./utils/Path.h"
#include "import/ImportHelpers.h"
#include "import/UrdfImporter.h"

#include <carb/PluginUtils.h>
#include <carb/logging/Log.h>

#include <omni/ext/IExt.h>
#include <omni/kit/IApp.h>
#include <omni/kit/IStageUpdate.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/usd/payloads.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/variantSets.h>
#include <pybind11/pybind11.h>

#include <fstream>
#include <memory>


using namespace carb;


const struct carb::PluginImplDesc kPluginImpl = { "isaacsim.asset.importer.urdf", "URDF Utilities", "NVIDIA",
                                                  carb::PluginHotReload::eEnabled, "dev" };

CARB_PLUGIN_IMPL(kPluginImpl, isaacsim::asset::importer::urdf::Urdf)
CARB_PLUGIN_IMPL_DEPS(omni::kit::IApp, carb::logging::ILogging)

namespace
{


isaacsim::asset::importer::urdf::UrdfRobot parseUrdfString(const std::string& urdf,
                                                           isaacsim::asset::importer::urdf::ImportConfig& importConfig)
{


    isaacsim::asset::importer::urdf::UrdfRobot robot;

    // std::string filename = assetRoot + "/" + assetName;
    {


        if (!parseUrdfString(urdf, robot))
        {
            CARB_LOG_ERROR("Failed to parse URDF string '%s'", urdf.c_str());
            return robot;
        }

        if (importConfig.mergeFixedJoints)
        {
            collapseFixedJoints(robot);
        }

        for (auto& joint : robot.joints)
        {
            // Parse the kinematics linked list.
            // joint.second.naturalStiffness = computeSimpleStiffness(robot, joint.first, kDesiredNaturalFrequency,1);
            auto& parent_link = robot.links.at(joint.second.parentLinkName);
            auto& child_link = robot.links.at(joint.second.childLinkName);

            parent_link.childrenLinks.push_back(child_link.name);
            child_link.parentLink = parent_link.name;
        }

        if (importConfig.collisionFromVisuals)
        {
            addVisualMeshToCollision(robot);
        }

        for (auto& joint : robot.joints)
        {


            joint.second.drive.targetType = importConfig.defaultDriveType;
            switch (joint.second.drive.targetType)
            {
            case isaacsim::asset::importer::urdf::UrdfJointTargetType::POSITION:
                joint.second.drive.strength =
                    computeSimpleStiffness(robot, joint.first, joint.second.drive.naturalFrequency);
                if (importConfig.overrideJointDynamics)
                {
                    joint.second.drive.damping = joint.second.jointInertia * joint.second.drive.dampingRatio * 2 *
                                                 joint.second.drive.naturalFrequency;
                }
                else
                {
                    joint.second.drive.damping = joint.second.dynamics.damping;
                }
                break;
            case isaacsim::asset::importer::urdf::UrdfJointTargetType::VELOCITY:
                if (importConfig.overrideJointDynamics)
                {
                    joint.second.drive.strength = importConfig.defaultDriveStrength;
                    joint.second.drive.damping = 0.0f;
                }
                break;
            case isaacsim::asset::importer::urdf::UrdfJointTargetType::NONE:
                joint.second.drive.strength = 0.0f;
                joint.second.drive.damping = 0.0f;
                break;
            default:
                CARB_LOG_ERROR("Unknown drive target type %d", (int)joint.second.drive.targetType);
                break;
            }
        }
    }
    return robot;
}

isaacsim::asset::importer::urdf::UrdfRobot parseUrdf(const std::string& assetRoot,
                                                     const std::string& assetName,
                                                     isaacsim::asset::importer::urdf::ImportConfig& importConfig)
{
    isaacsim::asset::importer::urdf::UrdfRobot robot;

    std::string filename = assetRoot + "/" + assetName;
    {


        CARB_LOG_INFO("Trying to import %s", filename.c_str());

        if (parseUrdf(assetRoot, assetName, robot))
        {
            robot.assetRoot = assetRoot;
            robot.urdfPath = assetName;
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
            if (joint.second.drive.targetType == isaacsim::asset::importer::urdf::UrdfJointTargetType::POSITION)
            {
                joint.second.drive.strength =
                    computeSimpleStiffness(robot, joint.first, joint.second.drive.naturalFrequency);
                if (importConfig.overrideJointDynamics && importConfig.defaultPositionDriveDamping > 0.0f)
                // set position gain
                {
                    joint.second.drive.damping = joint.second.jointInertia * 2 * joint.second.drive.naturalFrequency *
                                                 joint.second.drive.dampingRatio;
                }
                else
                {
                    joint.second.drive.damping = joint.second.dynamics.damping;
                }
            }
            else if (joint.second.drive.targetType == isaacsim::asset::importer::urdf::UrdfJointTargetType::VELOCITY)
            {
                // set position gain to zero so we can achieve velocity control
                // joint.second.dynamics.stiffness = 0.0f;
                joint.second.drive.strength = importConfig.defaultDriveStrength;
                if (importConfig.overrideJointDynamics)
                {
                    joint.second.drive.damping = 0.0f;
                }
            }
            else if (joint.second.drive.targetType == isaacsim::asset::importer::urdf::UrdfJointTargetType::NONE)
            {
                joint.second.drive.strength = 0.0f;
                joint.second.drive.damping = 0.0f;
            }
            else
            {
                CARB_LOG_ERROR("Unknown drive target type %d", (int)joint.second.drive.targetType);
            }
        }
    }
    return robot;
}


bool FileExists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

void OpenOrCreateNew(pxr::UsdStageRefPtr& stage, const std::string& stage_identifier)
{
    if (pxr::UsdStage::IsSupportedFile(stage_identifier))
    {
        if (FileExists(stage_identifier))
        {
            stage = pxr::UsdStage::Open(stage_identifier);
        }
        if (!stage)
        {
            CARB_LOG_INFO("Creating Stage: %s", stage_identifier.c_str());
            stage = pxr::UsdStage::CreateNew(stage_identifier);
        }
        else
        {
            stage->GetRootLayer()->Clear();
            stage->Save();
        }
    }
    else
    {
        CARB_LOG_ERROR("Stage identifier %s is not supported", stage_identifier.c_str());
    }
}

float computeJointNaturalStiffess(const isaacsim::asset::importer::urdf::UrdfRobot& robot,
                                  std::string joint,
                                  float naturalFrequency)
{
    if (robot.joints.find(joint) == robot.joints.end())
    {
        CARB_LOG_ERROR("Joint not found: %s", joint.c_str());
        return 0;
    }
    return computeSimpleStiffness(robot, joint, naturalFrequency);
}

std::string importRobot(const std::string& assetRoot,
                        const std::string& assetName,
                        const isaacsim::asset::importer::urdf::UrdfRobot& robot,
                        isaacsim::asset::importer::urdf::ImportConfig& importConfig,
                        const std::string& stage_identifier = "",
                        const bool getArticulationRoot = false)
{
    std::string asset_root = assetRoot;
    if (!robot.assetRoot.empty())
    {
        asset_root = robot.assetRoot;
    }
    std::string asset_name = assetName;
    if (!robot.urdfPath.empty())
    {
        asset_name = robot.urdfPath;
    }
    isaacsim::asset::importer::urdf::UrdfImporter urdfImporter(asset_root, asset_name, importConfig);
    bool save_stage = true;

    std::unordered_map<std::string, pxr::UsdStageRefPtr> stages;
    stages["stage"] = pxr::UsdStageRefPtr();
    stages["sensor_stage"] = pxr::UsdStageRefPtr();
    stages["physics_stage"] = pxr::UsdStageRefPtr();
    stages["base_stage"] = pxr::UsdStageRefPtr();
    bool multi_layer = true;
    std::string name = "";
    if (stage_identifier != "" && pxr::UsdStage::IsSupportedFile(stage_identifier))
    {
        OpenOrCreateNew(stages["stage"], stage_identifier);
        std::string directoryPath = pxr::TfGetPathName(stage_identifier);
        name = pxr::TfStringGetBeforeSuffix(pxr::TfGetBaseName(stage_identifier));

        // CARB_LOG_WARN(stage_identifier.c_str());
        // CARB_LOG_WARN(directoryPath.c_str());
        // CARB_LOG_WARN(name.c_str());

        OpenOrCreateNew(stages["sensor_stage"], directoryPath + "/configuration/" + name + "_sensor.usd");
        OpenOrCreateNew(stages["physics_stage"], directoryPath + "/configuration/" + name + "_physics.usd");
        OpenOrCreateNew(stages["base_stage"], directoryPath + "/configuration/" + name + "_base.usd");

        importConfig.makeDefaultPrim = true;
    }
    if (!stages["stage"]) // If all else fails, import on current stage
    {
        CARB_LOG_INFO("Importing URDF to Current Stage");
        // Get the 'active' USD stage from the USD stage cache.
        const std::vector<pxr::UsdStageRefPtr> allStages = pxr::UsdUtilsStageCache::Get().GetAllStages();
        if (allStages.size() != 1)
        {
            CARB_LOG_ERROR("Cannot determine the 'active' USD stage (%zu stages present in the USD stage cache).",
                           allStages.size());
            return "";
        }
        stages["stage"] = allStages[0];
        std::string identifier = pxr::TfStringGetBeforeSuffix((stages["stage"]->GetRootLayer()->GetIdentifier()));
        if (pxr::TfStringStartsWith(identifier, "anon:"))
        {
            CARB_LOG_WARN("Creating Asset in an in-memory stage, will not create layered structure");
            stages["sensor_stage"] = stages["stage"];
            stages["physics_stage"] = stages["stage"];
            stages["base_stage"] = stages["stage"];
            multi_layer = false;
            save_stage = false;
        }
        else
        {
            std::string directoryPath = pxr::TfGetPathName(identifier);
            std::string name = pxr::TfGetBaseName(identifier);
            OpenOrCreateNew(stages["sensor_stage"], directoryPath + "/configuration/" + name + "_sensor.usd");
            OpenOrCreateNew(stages["physics_stage"], directoryPath + "/configuration/" + name + "_physics.usd");
            OpenOrCreateNew(stages["base_stage"], directoryPath + "/configuration/" + name + "_base.usd");
        }
    }

    for (auto& stage : stages)
    {

        {
            pxr::UsdGeomSetStageUpAxis(stage.second, pxr::UsdGeomTokens->z);
            pxr::UsdGeomSetStageMetersPerUnit(stage.second, 1.0 / importConfig.distanceScale);
        }
    }

    pxr::SdfLayerRefPtr rootLayer = stages["stage"]->GetRootLayer();
    if (multi_layer)
    {
        auto subLayerPaths = rootLayer->GetSubLayerPaths();
        auto sensor_layer = resolve_relative(
            stages["stage"]->GetRootLayer()->GetIdentifier(), stages["sensor_stage"]->GetRootLayer()->GetIdentifier());
        if (std::find(subLayerPaths.begin(), subLayerPaths.end(), sensor_layer) == subLayerPaths.end())
        {
            subLayerPaths.push_back(sensor_layer);
        }
        auto physics_layer = resolve_relative(
            stages["stage"]->GetRootLayer()->GetIdentifier(), stages["physics_stage"]->GetRootLayer()->GetIdentifier());
        if (std::find(subLayerPaths.begin(), subLayerPaths.end(), physics_layer) == subLayerPaths.end())
        {
            subLayerPaths.push_back(physics_layer);
        }
        auto physics_subLayerPaths = stages["physics_stage"]->GetRootLayer()->GetSubLayerPaths();
        auto base_layer = resolve_relative(stages["physics_stage"]->GetRootLayer()->GetIdentifier(),
                                           stages["base_stage"]->GetRootLayer()->GetIdentifier());
        if (std::find(physics_subLayerPaths.begin(), physics_subLayerPaths.end(), base_layer) ==
            physics_subLayerPaths.end())
        {
            physics_subLayerPaths.push_back(base_layer);
        }
    }
    // Add as sublayers for authoring
    std::string result = "";
    if (stages["stage"])
    {
        pxr::UsdGeomSetStageMetersPerUnit(stages["stage"], 1.0 / importConfig.distanceScale);
        result = urdfImporter.addToStage(stages, robot, getArticulationRoot);
        if (!result.empty() && save_stage) // Only set up variant break down if saving asset, otherwise information gets
                                           // lost and simualtion won't work
        {
            CARB_LOG_INFO("Saving Sensor Stage %s", stages["sensor_stage"]->GetRootLayer()->GetIdentifier().c_str());
            stages["sensor_stage"]->Save();
            CARB_LOG_INFO("Saving Physics Stage %s", stages["physics_stage"]->GetRootLayer()->GetIdentifier().c_str());
            stages["physics_stage"]->Save();
            CARB_LOG_INFO("Saving Base Stage %s", stages["base_stage"]->GetRootLayer()->GetIdentifier().c_str());
            stages["base_stage"]->Save();
            CARB_LOG_INFO("Saving Stage %s", stages["stage"]->GetRootLayer()->GetIdentifier().c_str());
            stages["stage"]->Save();
            // _stage->Export(stage_identifier);

            // Remove the physics and sensor stages from sublayers, and add them as payloads through variants
            rootLayer->GetSubLayerPaths().clear();
            auto root_prim = stages["stage"]->GetPrimAtPath(pxr::SdfPath(result));
            if (!root_prim && !result.empty())
            {
                root_prim = stages["stage"]->DefinePrim(pxr::SdfPath(result), pxr::TfToken("Xform"));
            }
            if (!root_prim)
            {
                return result;
            }
            // ensure the root prim is the default prim
            // stages["stage"]->SetDefaultPrim(root_prim);


            pxr::UsdVariantSets variantSets = root_prim.GetVariantSets();
            pxr::UsdVariantSet physics = variantSets.AddVariantSet("Physics");
            physics.AddVariant("None");
            physics.SetVariantSelection("None");
            {
                pxr::UsdEditContext ctxt(physics.GetVariantEditContext());
                root_prim.GetReferences().AddReference(
                    resolve_relative(stages["stage"]->GetRootLayer()->GetIdentifier(),
                                     stages["base_stage"]->GetRootLayer()->GetIdentifier()));
                auto joints = stages["stage"]->GetPrimAtPath(root_prim.GetPath().AppendPath(pxr::SdfPath("joints")));
                if (joints)
                {
                    joints.SetActive(false);
                }
                auto loop_joints =
                    stages["stage"]->GetPrimAtPath(root_prim.GetPath().AppendPath(pxr::SdfPath("loop_joints")));
                if (loop_joints)
                {
                    loop_joints.SetActive(false);
                }
                auto root_joint =
                    stages["stage"]->GetPrimAtPath(root_prim.GetPath().AppendPath(pxr::SdfPath("root_joint")));
                if (root_joint)
                {
                    root_joint.SetActive(false);
                }
            }
            physics.AddVariant("PhysX");
            physics.SetVariantSelection("PhysX");
            {
                pxr::UsdEditContext ctxt(physics.GetVariantEditContext());
                root_prim.GetPayloads().AddPayload(
                    pxr::SdfPayload(resolve_relative(stages["stage"]->GetRootLayer()->GetIdentifier(),
                                                     stages["physics_stage"]->GetRootLayer()->GetIdentifier())));
            }


            pxr::UsdVariantSet sensor = variantSets.AddVariantSet("Sensor");
            sensor.AddVariant("None");
            sensor.AddVariant("Sensors");
            sensor.SetVariantSelection("Sensors");
            {
                pxr::UsdEditContext ctxt(sensor.GetVariantEditContext());
                root_prim.GetPayloads().AddPayload(
                    pxr::SdfPayload(resolve_relative(stages["stage"]->GetRootLayer()->GetIdentifier(),
                                                     stages["sensor_stage"]->GetRootLayer()->GetIdentifier())));
            }
            CARB_LOG_INFO("Import Done, saving");

            stages["stage"]->Save();
            // _stage->Export(stage_identifier);
        }
    }
    else
    {
        CARB_LOG_ERROR("Stage pointer not valid, could not import urdf to stage");
    }
    return result;
}
}


pybind11::list addLinksAndJoints(isaacsim::asset::importer::urdf::KinematicChain::Node* parentNode)
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

pybind11::dict getKinematicChain(const isaacsim::asset::importer::urdf::UrdfRobot& robot)
{
    pybind11::dict robotDict;
    isaacsim::asset::importer::urdf::KinematicChain chain;
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


void fillInterface(isaacsim::asset::importer::urdf::Urdf& iface)
{
    using namespace isaacsim::asset::importer::urdf;
    memset(&iface, 0, sizeof(iface));
    iface.parseUrdf = parseUrdf;
    iface.parseUrdfString = parseUrdfString;
    iface.importRobot = importRobot;
    iface.getKinematicChain = getKinematicChain;
    iface.computeJointNaturalStiffess = computeJointNaturalStiffess;
}
