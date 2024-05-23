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

#include "UrdfImporter.h"

#include "../core/PathUtils.h"
#include "UrdfImporter.h"

// #include "../../GymJoint.h"
// #include "../../helpers.h"


#include "ImportHelpers.h"
#include "assimp/Importer.hpp"
#include "assimp/cfileio.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include <carb/logging/Log.h>

#include <omni/ext/ExtensionsUtils.h>
#include <omni/ext/IExtensions.h>
#include <omni/kit/IApp.h>
#include <physicsSchemaTools/UsdTools.h>
#include <physxSchema/jointStateAPI.h>
#include <physxSchema/physxArticulationAPI.h>
#include <physxSchema/physxCollisionAPI.h>
#include <physxSchema/physxJointAPI.h>
#include <physxSchema/physxMimicJointAPI.h>
#include <physxSchema/physxSceneAPI.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdPhysics/articulationRootAPI.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/driveAPI.h>
#include <pxr/usd/usdPhysics/fixedJoint.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/meshCollisionAPI.h>
#include <pxr/usd/usdPhysics/prismaticJoint.h>
#include <pxr/usd/usdPhysics/revoluteJoint.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>
#include <pxr/usd/usdPhysics/sphericalJoint.h>
#include <pxr/usd/usdUtils/authoring.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <OmniClient.h>
#include <cmath>
#include <fstream>
#include <iostream>

namespace
{
constexpr float kNegligibleMass = 1.0e-6f;
constexpr float kSmallEps = 1.0e-5f;
}
namespace omni
{
namespace importer
{
namespace urdf
{


void saveJsonToFile(const rapidjson::Document& jsonDoc, const std::string& filename)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    jsonDoc.Accept(writer);

    std::ofstream outFile(filename);
    if (outFile.is_open())
    {
        outFile << buffer.GetString();
        outFile.close();
        CARB_LOG_INFO("JSON saved to %s", filename.c_str());
    }
    else
    {
        CARB_LOG_ERROR("Error saving JSON to file: %s", filename.c_str());
    }
}

UrdfRobot UrdfImporter::createAsset()
{
    UrdfRobot robot;
    if (!parseUrdf(assetRoot_, urdfPath_, robot))
    {
        CARB_LOG_ERROR("Failed to parse URDF file '%s'", urdfPath_.c_str());
        return robot;
    }

    if (config.mergeFixedJoints)
    {
        collapseFixedJoints(robot);
    }

    if (config.collisionFromVisuals)
    {
        addVisualMeshToCollision(robot);
    }

    return robot;
}

const char* subdivisionschemes[4] = { "catmullClark", "loop", "bilinear", "none" };

pxr::UsdPrim FindPrimByNameAndType(const pxr::UsdStageRefPtr& stage,
                                   const std::string& primName,
                                   const pxr::TfType& primType)
{
    // Define the root path for traversal
    pxr::SdfPath rootPath = pxr::SdfPath::AbsoluteRootPath();

    // Define the range for traversal with type filtering
    pxr::UsdPrimRange range(stage->GetPrimAtPath(rootPath));

    // Iterate over prims within the range
    for (const pxr::UsdPrim& prim : range)
    {
        // Check if the prim matches the given name
        if (prim.GetName() == primName && prim.IsA(primType))
        {
            return prim;
        }
    }
    // If the prim is not found, return an invalid prim
    return pxr::UsdPrim();
}

pxr::UsdPrim addMesh(pxr::UsdStageWeakPtr stage,
                     UrdfGeometry geometry,
                     std::string assetRoot,
                     std::string urdfPath,
                     std::string name,
                     Transform origin,
                     const bool loadMaterials,
                     const double distanceScale,
                     const bool flipVisuals,
                     std::map<pxr::TfToken, std::string>& materialsList,
                     const char* subdivisionScheme,
                     const bool instanceable = false,
                     const bool replaceCylindersWithCapsules = false)
{
    pxr::SdfPath path;
    if (geometry.type == UrdfGeometryType::MESH)
    {
        std::string meshUri = geometry.meshFilePath;
        std::string meshPath = resolveXrefPath(assetRoot, urdfPath, meshUri);

        // pxr::GfMatrix4d meshMat;
        if (meshPath.empty())
        {
            CARB_LOG_WARN("Failed to resolve mesh '%s'", meshUri.c_str());
            return pxr::UsdPrim(); // move to next shape
        }
        // mesh is a usd file, add as a reference directly to a new xform
        else if (IsUsdFile(meshPath))
        {
            CARB_LOG_INFO("Adding Usd reference '%s'", meshPath.c_str());
            path = pxr::SdfPath(omni::importer::urdf::GetNewSdfPathString(stage, name));
            pxr::UsdGeomXform usdXform = pxr::UsdGeomXform::Define(stage, path);
            usdXform.GetPrim().GetReferences().AddReference(meshPath);
        }
        else
        {
            CARB_LOG_INFO("Found Mesh At: %s", meshPath.c_str());
            static auto propsDeleter = [](aiPropertyStore* props)
            {
                if (props)
                {
                    aiReleasePropertyStore(props);
                }
            };

            auto props = std::shared_ptr<aiPropertyStore>(aiCreatePropertyStore(), propsDeleter);
            aiSetImportPropertyInteger(
                props.get(), AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
            aiSetImportPropertyInteger(props.get(), AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, 0);


            auto assimpScene = aiImportFileExWithProperties(
                meshPath.c_str(),
                aiProcess_GenSmoothNormals | aiProcess_RemoveRedundantMaterials | aiProcess_GenUVCoords |
                    aiProcess_TransformUVCoords | aiProcess_JoinIdenticalVertices,
                nullptr, props.get());

            // aiImportFile(meshPath.c_str(), aiProcess_GenSmoothNormals |
            //                                    aiProcess_RemoveRedundantMaterials | aiProcess_GlobalScale);
            static auto sceneDeleter = [](const aiScene* scene)
            {
                if (scene)
                {
                    aiReleaseImport(scene);
                }
            };

            if (!assimpScene || !assimpScene->HasMeshes())
            {
                CARB_LOG_WARN("Unable to load file %s", meshPath.c_str());
            }
            auto sceneRAII = std::shared_ptr<const aiScene>(assimpScene, sceneDeleter);
            // Add visuals
            if (!sceneRAII || !sceneRAII->mRootNode)
            {
                CARB_LOG_WARN("Asset convert failed as asset file is broken.");
            }
            else if (sceneRAII->mNumMeshes == 0)
            {
                CARB_LOG_WARN("Asset convert failed as asset cannot be loaded.");
            }
            else
            {
                path = SimpleImport(stage, name, sceneRAII.get(), meshPath, materialsList, loadMaterials, flipVisuals,
                                    subdivisionScheme, instanceable);
            }
        }
    }
    else if (geometry.type == UrdfGeometryType::SPHERE)
    {
        pxr::UsdGeomSphere gprim = pxr::UsdGeomSphere::Define(stage, pxr::SdfPath(name));
        pxr::VtVec3fArray extentArray(2);

        gprim.ComputeExtent(geometry.radius, &extentArray);
        gprim.GetExtentAttr().Set(extentArray);
        gprim.GetRadiusAttr().Set(double(geometry.radius));
        path = pxr::SdfPath(name);
    }
    else if (geometry.type == UrdfGeometryType::BOX)
    {
        pxr::UsdGeomCube gprim = pxr::UsdGeomCube::Define(stage, pxr::SdfPath(name));
        pxr::VtVec3fArray extentArray(2);
        extentArray[1] = pxr::GfVec3f(geometry.size_x * 0.5f, geometry.size_y * 0.5f, geometry.size_z * 0.5f);
        extentArray[0] = -extentArray[1];
        gprim.GetExtentAttr().Set(extentArray);
        gprim.GetSizeAttr().Set(1.0);
        path = pxr::SdfPath(name);
    }
    else if (geometry.type == UrdfGeometryType::CYLINDER && !replaceCylindersWithCapsules)
    {

        pxr::UsdGeomCylinder gprim = pxr::UsdGeomCylinder::Define(stage, pxr::SdfPath(name));
        pxr::VtVec3fArray extentArray(2);
        gprim.ComputeExtent(geometry.length, geometry.radius, pxr::UsdGeomTokens->z, &extentArray);
        gprim.GetAxisAttr().Set(pxr::UsdGeomTokens->z);
        gprim.GetExtentAttr().Set(extentArray);
        gprim.GetHeightAttr().Set(double(geometry.length));
        gprim.GetRadiusAttr().Set(double(geometry.radius));
        path = pxr::SdfPath(name);
    }
    else if (geometry.type == UrdfGeometryType::CAPSULE ||
             (geometry.type == UrdfGeometryType::CYLINDER && replaceCylindersWithCapsules))
    {
        pxr::UsdGeomCapsule gprim = pxr::UsdGeomCapsule::Define(stage, pxr::SdfPath(name));
        pxr::VtVec3fArray extentArray(2);
        gprim.ComputeExtent(geometry.length, geometry.radius, pxr::UsdGeomTokens->z, &extentArray);
        gprim.GetAxisAttr().Set(pxr::UsdGeomTokens->z);
        gprim.GetExtentAttr().Set(extentArray);
        gprim.GetHeightAttr().Set(double(geometry.length));
        gprim.GetRadiusAttr().Set(double(geometry.radius));
        path = pxr::SdfPath(name);
    }

    pxr::UsdPrim prim = stage->GetPrimAtPath(path);
    if (prim)
    {
        Transform transform = origin;

        pxr::GfVec3d scale;
        if (geometry.type == UrdfGeometryType::MESH)
        {
            scale = pxr::GfVec3d(geometry.scale_x, geometry.scale_y, geometry.scale_z);
        }
        else if (geometry.type == UrdfGeometryType::BOX)
        {
            scale = pxr::GfVec3d(geometry.size_x, geometry.size_y, geometry.size_z);
        }
        else
        {
            scale = pxr::GfVec3d(1, 1, 1);
        }
        pxr::UsdGeomXformable gprim = pxr::UsdGeomXformable(prim);
        gprim.ClearXformOpOrder();
        gprim.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(distanceScale * pxr::GfVec3d(transform.p.x, transform.p.y, transform.p.z));
        gprim.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(pxr::GfQuatd(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        gprim.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(distanceScale * scale);
    }
    return prim;
}

void UrdfImporter::buildInstanceableStage(pxr::UsdStageRefPtr stage,
                                          const KinematicChain::Node* parentNode,
                                          const std::string& robotBasePath,
                                          const UrdfRobot& urdfRobot)
{
    if (parentNode->parentJointName_ == "")
    {
        const UrdfLink& urdfLink = urdfRobot.links.at(parentNode->linkName_);
        addInstanceableMeshes(stage, urdfLink, robotBasePath, urdfRobot);
    }
    if (!parentNode->childNodes_.empty())
    {
        for (const auto& childNode : parentNode->childNodes_)
        {
            if (urdfRobot.joints.find(childNode->parentJointName_) != urdfRobot.joints.end())
            {
                if (urdfRobot.links.find(childNode->linkName_) != urdfRobot.links.end())
                {
                    const UrdfLink& childLink = urdfRobot.links.at(childNode->linkName_);
                    addInstanceableMeshes(stage, childLink, robotBasePath, urdfRobot);
                    // Recurse through the links children
                    buildInstanceableStage(stage, childNode.get(), robotBasePath, urdfRobot);
                }
            }
        }
    }
}

void UrdfImporter::addInstanceableMeshes(pxr::UsdStageRefPtr stage,
                                         const UrdfLink& link,
                                         const std::string& robotBasePath,
                                         const UrdfRobot& robot)
{
    std::map<std::string, std::string> linkMatPrimPaths;
    std::map<pxr::TfToken, std::string> linkMaterialList;

    // Add visuals
    for (size_t i = 0; i < link.visuals.size(); i++)
    {
        std::string meshName;
        std::string name = "mesh_" + std::to_string(i);
        if (link.visuals[i].name.size() > 0)
        {
            name = link.visuals[i].name;
        }
        meshName = robotBasePath + link.name + "/visuals/" + name;

        bool loadMaterial = true;

        auto mat = link.visuals[i].material;
        auto urdfMatIter = robot.materials.find(link.visuals[i].material.name);
        if (urdfMatIter != robot.materials.end())
        {
            mat = urdfMatIter->second;
        }

        auto& color = mat.color;
        if (color.r >= 0 && color.g >= 0 && color.b >= 0)
        {
            loadMaterial = false;
        }

        pxr::UsdPrim prim = addMesh(stage, link.visuals[i].geometry, assetRoot_, urdfPath_, meshName,
                                    link.visuals[i].origin, loadMaterial, config.distanceScale, false, linkMaterialList,
                                    subdivisionschemes[(int)config.subdivisionScheme], true);

        if (prim)
        {
            if (loadMaterial == false)
            {
                // This Material was already created for this link, reuse
                auto urdfMatIter = linkMatPrimPaths.find(link.visuals[i].material.name);
                if (urdfMatIter != linkMatPrimPaths.end())
                {
                    std::string path = linkMatPrimPaths[link.visuals[i].material.name];

                    auto matPrim = stage->GetPrimAtPath(pxr::SdfPath(path));

                    if (matPrim)
                    {
                        auto shadePrim = pxr::UsdShadeMaterial(matPrim);
                        if (shadePrim)
                        {
                            pxr::UsdShadeMaterialBindingAPI mbi(prim);
                            mbi.Bind(shadePrim);
                        }
                    }
                }
                else
                {
                    auto& color = link.visuals[i].material.color;
                    std::stringstream ss;
                    ss << std::uppercase << std::hex << (int)(256 * color.r) << std::uppercase << std::hex
                       << (int)(256 * color.g) << std::uppercase << std::hex << (int)(256 * color.b);
                    std::pair<std::string, UrdfMaterial> mat_pair(ss.str(), link.visuals[i].material);

                    pxr::UsdShadeMaterial matPrim = addMaterial(stage, mat_pair, prim.GetPath().GetParentPath());
                    std::string matName = link.visuals[i].material.name;
                    std::string matPrimName = matName == "" ? mat_pair.first : matName;
                    linkMatPrimPaths[matName] =
                        prim.GetPath()
                            .GetParentPath()
                            .AppendPath(pxr::SdfPath("Looks/" + makeValidUSDIdentifier("material_" + name)))
                            .GetString();
                    if (matPrim)
                    {
                        pxr::UsdShadeMaterialBindingAPI mbi(prim);
                        mbi.Bind(matPrim);
                    }
                }
            }
        }
        else
        {
            CARB_LOG_WARN("Prim %s not created", meshName.c_str());
        }
    }
    // Add collisions
    CARB_LOG_INFO("Add collisions: %s", link.name.c_str());
    for (size_t i = 0; i < link.collisions.size(); i++)
    {
        std::string meshName;
        std::string name = "mesh_" + std::to_string(i);
        if (link.collisions[i].name.size() > 0)
        {
            name = link.collisions[i].name;
        }
        meshName = robotBasePath + link.name + "/collisions/" + name;

        pxr::UsdPrim prim =
            addMesh(stage, link.collisions[i].geometry, assetRoot_, urdfPath_, meshName, link.collisions[i].origin,
                    false, config.distanceScale, false, materialsList,
                    subdivisionschemes[(int)config.subdivisionScheme], false, config.replaceCylindersWithCapsules);
        // Enable collisions on prim
        if (prim)
        {
            pxr::UsdPhysicsCollisionAPI::Apply(prim);
            if (link.collisions[i].geometry.type == UrdfGeometryType::SPHERE)
            {
                pxr::UsdPhysicsCollisionAPI::Apply(prim);
            }
            else if (link.collisions[i].geometry.type == UrdfGeometryType::BOX)
            {
                pxr::UsdPhysicsCollisionAPI::Apply(prim);
            }
            else if (link.collisions[i].geometry.type == UrdfGeometryType::CYLINDER)
            {
                pxr::UsdPhysicsCollisionAPI::Apply(prim);
            }
            else if (link.collisions[i].geometry.type == UrdfGeometryType::CAPSULE)
            {
                pxr::UsdPhysicsCollisionAPI::Apply(prim);
            }
            else
            {
                pxr::UsdPhysicsMeshCollisionAPI physicsMeshAPI = pxr::UsdPhysicsMeshCollisionAPI::Apply(prim);
                if (config.convexDecomp == true)
                {
                    physicsMeshAPI.CreateApproximationAttr().Set(pxr::UsdPhysicsTokens.Get()->convexDecomposition);
                }
                else
                {
                    physicsMeshAPI.CreateApproximationAttr().Set(pxr::UsdPhysicsTokens.Get()->convexHull);
                }
            }
            pxr::UsdGeomMesh(prim).CreatePurposeAttr().Set(pxr::UsdGeomTokens->guide);
        }
        else
        {
            CARB_LOG_WARN("Prim %s not created", meshName.c_str());
        }
    }
}

void UrdfImporter::addRigidBody(pxr::UsdStageWeakPtr stage,
                                const UrdfLink& link,
                                const Transform& poseBodyToWorld,
                                pxr::UsdGeomXform robotPrim,
                                const UrdfRobot& robot)
{
    std::string robotBasePath = robotPrim.GetPath().GetString() + "/";
    CARB_LOG_INFO("Add Rigid Body: %s", link.name.c_str());
    // Create Link Prim
    auto linkPrim = pxr::UsdGeomXform::Define(stage, pxr::SdfPath(robotBasePath + link.name));
    if (linkPrim)
    {
        Transform transform = poseBodyToWorld; // urdfOriginToTransform(link.inertial.origin);

        linkPrim.ClearXformOpOrder();

        linkPrim.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(config.distanceScale * pxr::GfVec3d(transform.p.x, transform.p.y, transform.p.z));
        linkPrim.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(pxr::GfQuatd(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        linkPrim.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(1, 1, 1));

        for (const auto& pair : link.mergedChildren)
        {
            auto childXform = pxr::UsdGeomXform::Define(stage, linkPrim.GetPath().AppendPath(pxr::SdfPath(pair.first)));
            if (childXform)
            {
                childXform.ClearXformOpOrder();

                childXform.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble)
                    .Set(config.distanceScale * pxr::GfVec3d(pair.second.p.x, pair.second.p.y, pair.second.p.z));
                childXform.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble)
                    .Set(pxr::GfQuatd(pair.second.q.w, pair.second.q.x, pair.second.q.y, pair.second.q.z));
                childXform.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(1, 1, 1));
            }
        }

        pxr::UsdPhysicsRigidBodyAPI physicsAPI = pxr::UsdPhysicsRigidBodyAPI::Apply(linkPrim.GetPrim());

        pxr::UsdPhysicsMassAPI massAPI = pxr::UsdPhysicsMassAPI::Apply(linkPrim.GetPrim());
        if (link.inertial.hasMass)
        {
            massAPI.CreateMassAttr().Set(link.inertial.mass);
        }
        else if (config.density > 0)
        {
            // scale from kg/m^2 to specified units
            massAPI.CreateDensityAttr().Set(config.density);
        }
        else
        {
            massAPI.CreateMassAttr().Set(kNegligibleMass); // Set mass significantly low that it won't impact
                                                           // simulation, but not so low as to be treated as massless
        }

        if (link.inertial.hasInertia && config.importInertiaTensor)
        {
            Matrix33 inertiaMatrix;
            inertiaMatrix.cols[0] = Vec3(link.inertial.inertia.ixx, link.inertial.inertia.ixy, link.inertial.inertia.ixz);
            inertiaMatrix.cols[1] = Vec3(link.inertial.inertia.ixy, link.inertial.inertia.iyy, link.inertial.inertia.iyz);
            inertiaMatrix.cols[2] = Vec3(link.inertial.inertia.ixz, link.inertial.inertia.iyz, link.inertial.inertia.izz);

            Quat principalAxes;
            Vec3 diaginertia = Diagonalize(inertiaMatrix, principalAxes);

            // input is meters, but convert to kit units
            massAPI.CreateDiagonalInertiaAttr().Set(config.distanceScale * config.distanceScale *
                                                    pxr::GfVec3f(diaginertia.x, diaginertia.y, diaginertia.z));
            massAPI.CreatePrincipalAxesAttr().Set(
                pxr::GfQuatf(principalAxes[3], principalAxes[0], principalAxes[1], principalAxes[2]));
        }

        if (link.inertial.hasOrigin)
        {
            massAPI.CreateCenterOfMassAttr().Set(pxr::GfVec3f(float(config.distanceScale * link.inertial.origin.p.x),
                                                              float(config.distanceScale * link.inertial.origin.p.y),
                                                              float(config.distanceScale * link.inertial.origin.p.z)));
        }
    }
    else
    {
        CARB_LOG_WARN("linkPrim %s not created", link.name.c_str());
        return;
    }

    // Add visuals
    if (config.makeInstanceable && link.visuals.size() > 0)
    {
        pxr::SdfPath visualPrimPath = pxr::SdfPath(robotBasePath + link.name + "/visuals");
        pxr::UsdPrim visualPrim = stage->DefinePrim(visualPrimPath);
        visualPrim.GetReferences().AddReference(config.instanceableMeshUsdPath, visualPrimPath);
        visualPrim.SetInstanceable(true);
    }
    else
    {
        for (size_t i = 0; i < link.visuals.size(); i++)
        {
            std::string meshName;
            if (link.visuals.size() > 1)
            {
                std::string name = "mesh_" + std::to_string(i);
                if (link.visuals[i].name.size() > 0)
                {
                    name = link.visuals[i].name;
                }
                meshName = robotBasePath + link.name + "/visuals/" + name;
            }
            else
            {
                meshName = robotBasePath + link.name + "/visuals";
            }
            bool loadMaterial = true;

            auto mat = link.visuals[i].material;
            auto urdfMatIter = robot.materials.find(link.visuals[i].material.name);
            if (urdfMatIter != robot.materials.end())
            {
                mat = urdfMatIter->second;
            }

            auto& color = mat.color;
            if (color.r >= 0 && color.g >= 0 && color.b >= 0)
            {
                loadMaterial = false;
            }

            pxr::UsdPrim prim = addMesh(stage, link.visuals[i].geometry, assetRoot_, urdfPath_, meshName,
                                        link.visuals[i].origin, loadMaterial, config.distanceScale, false,
                                        materialsList, subdivisionschemes[(int)config.subdivisionScheme]);

            if (prim)
            {

                if (loadMaterial == false)
                {
                    // This Material was in the master list, reuse
                    auto urdfMatIter = robot.materials.find(link.visuals[i].material.name);
                    if (urdfMatIter != robot.materials.end())
                    {
                        std::string path = matPrimPaths[link.visuals[i].material.name];

                        auto matPrim = stage->GetPrimAtPath(pxr::SdfPath(path));

                        if (matPrim)
                        {
                            auto shadePrim = pxr::UsdShadeMaterial(matPrim);
                            if (shadePrim)
                            {
                                pxr::UsdShadeMaterialBindingAPI mbi(prim);
                                mbi.Bind(shadePrim);
                            }
                        }
                    }
                    else
                    {
                        auto& color = link.visuals[i].material.color;
                        std::stringstream ss;
                        ss << std::uppercase << std::hex << (int)(256 * color.r) << std::uppercase << std::hex
                           << (int)(256 * color.g) << std::uppercase << std::hex << (int)(256 * color.b);
                        std::pair<std::string, UrdfMaterial> mat_pair(ss.str(), link.visuals[i].material);

                        pxr::UsdShadeMaterial matPrim =
                            addMaterial(stage, mat_pair, prim.GetPath().GetParentPath().GetParentPath());
                        if (matPrim)
                        {
                            pxr::UsdShadeMaterialBindingAPI mbi(prim);
                            mbi.Bind(matPrim);
                        }
                    }
                }
            }
            else
            {
                CARB_LOG_WARN("Prim %s not created", meshName.c_str());
            }
        }
    }
    // Add collisions
    CARB_LOG_INFO("Add collisions: %s", link.name.c_str());
    if (config.makeInstanceable && link.collisions.size() > 0)
    {
        pxr::SdfPath collisionPrimPath = pxr::SdfPath(robotBasePath + link.name + "/collisions");
        pxr::UsdPrim collisionPrim = stage->DefinePrim(collisionPrimPath);
        collisionPrim.GetReferences().AddReference(config.instanceableMeshUsdPath, collisionPrimPath);
        collisionPrim.SetInstanceable(true);
    }
    else
    {
        for (size_t i = 0; i < link.collisions.size(); i++)
        {

            std::string meshName;
            if (link.collisions.size() > 1 || config.makeInstanceable)
            {
                std::string name = "mesh_" + std::to_string(i);
                if (link.collisions[i].name.size() > 0)
                {
                    name = link.collisions[i].name;
                }
                meshName = robotBasePath + link.name + "/collisions/" + name;
            }
            else
            {
                meshName = robotBasePath + link.name + "/collisions";
            }

            pxr::UsdPrim prim =
                addMesh(stage, link.collisions[i].geometry, assetRoot_, urdfPath_, meshName, link.collisions[i].origin,
                        false, config.distanceScale, false, materialsList,
                        subdivisionschemes[(int)config.subdivisionScheme], false, config.replaceCylindersWithCapsules);
            // Enable collisions on prim
            if (prim)
            {
                pxr::UsdPhysicsCollisionAPI::Apply(prim);
                if (link.collisions[i].geometry.type == UrdfGeometryType::SPHERE)
                {
                    pxr::UsdPhysicsCollisionAPI::Apply(prim);
                }
                else if (link.collisions[i].geometry.type == UrdfGeometryType::BOX)
                {
                    pxr::UsdPhysicsCollisionAPI::Apply(prim);
                }
                else if (link.collisions[i].geometry.type == UrdfGeometryType::CYLINDER)
                {
                    pxr::UsdPhysicsCollisionAPI::Apply(prim);
                }
                else if (link.collisions[i].geometry.type == UrdfGeometryType::CAPSULE)
                {
                    pxr::UsdPhysicsCollisionAPI::Apply(prim);
                }
                else
                {
                    pxr::UsdPhysicsMeshCollisionAPI physicsMeshAPI = pxr::UsdPhysicsMeshCollisionAPI::Apply(prim);
                    if (config.convexDecomp == true)
                    {
                        physicsMeshAPI.CreateApproximationAttr().Set(pxr::UsdPhysicsTokens.Get()->convexDecomposition);
                    }
                    else
                    {
                        physicsMeshAPI.CreateApproximationAttr().Set(pxr::UsdPhysicsTokens.Get()->convexHull);
                    }
                }
                pxr::UsdGeomMesh(prim).CreatePurposeAttr().Set(pxr::UsdGeomTokens->guide);
            }
            else
            {
                CARB_LOG_WARN("Prim %s not created", meshName.c_str());
            }
        }
    }
    if (link.collisions.size() == 0)
    {
        if (!link.inertial.hasInertia || !config.importInertiaTensor)
        {
            CARB_LOG_WARN(
                "Link %s has no colliders, and no inertia was imported; assigning a small isotropic inertia matrix",
                link.name.c_str());

            pxr::UsdPhysicsMassAPI massAPI = pxr::UsdPhysicsMassAPI(linkPrim.GetPrim());
            massAPI.CreateDiagonalInertiaAttr().Set(config.distanceScale * config.distanceScale * 10.0f *
                                                    pxr::GfVec3f(kNegligibleMass, kNegligibleMass, kNegligibleMass));
        }
    }
    // Add Sensors
    for (auto& camera : link.cameras)
    {
        auto cameraPrim = pxr::UsdGeomCamera::Define(
            stage, pxr::SdfPath(linkPrim.GetPath().AppendChild(PXR_NS::TfToken(camera.name))));
        auto cameraXform = pxr::UsdGeomXformable(cameraPrim.GetPrim());
        cameraXform.ClearXformOpOrder();
        // TODO: not sure OV's default optical axis is the same as ROS, so we may need to transform the orientation from
        // URDF
        cameraXform.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(config.distanceScale * pxr::GfVec3d(camera.origin.p.x, camera.origin.p.y, camera.origin.p.z));
        cameraXform.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(pxr::GfQuatd(camera.origin.q.w, camera.origin.q.x, camera.origin.q.y, camera.origin.q.z));
        cameraXform.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(1, 1, 1));
        cameraPrim.GetClippingRangeAttr().Set(config.distanceScale * pxr::GfVec2f(camera.clipNear, camera.clipFar));

        // Compute Focal Length Assuming a fixed horizontal aperture of 20.955mm (default in cameras created with
        // Omniverse, which is the equivalent of the standard 35mm spherical projector aperture)
        // And a non-distorted camera sensor (no default distortion model is available in official URDF spec)
        float aperture = 0;
        cameraPrim.GetHorizontalApertureAttr().Get(&aperture);
        float focal = aperture / (2 * tan(camera.hfov / 2));

        cameraPrim.GetFocalLengthAttr().Set(focal);
    }


    for (auto& lidar : link.lidars)
    {
        // Add a RTX Lidar camera and wire so it can load json config files
        auto lidarPrim =
            pxr::UsdGeomCamera::Define(stage, pxr::SdfPath(linkPrim.GetPath().AppendChild(PXR_NS::TfToken(lidar.name))));
        auto lidarXform = pxr::UsdGeomXformable(lidarPrim.GetPrim());
        lidarXform.ClearXformOpOrder();
        // TODO: not sure OV's default optical axis is the same as ROS, so we may need to transform the orientation from
        // URDF
        lidarXform.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(config.distanceScale * pxr::GfVec3d(lidar.origin.p.x, lidar.origin.p.y, lidar.origin.p.z));
        lidarXform.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble)
            .Set(pxr::GfQuatd(lidar.origin.q.w, lidar.origin.q.x, lidar.origin.q.y, lidar.origin.q.z));
        lidarXform.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(1, 1, 1));
        lidarPrim.GetClippingRangeAttr().Set(config.distanceScale * pxr::GfVec2f(0.001f, 1000.0f));

        // Lazy Apply the IsaacRtxLidarSensorAPI to the prim
        // Get the SdfPrimSpec for the prim
        pxr::SdfTokenListOp schemasListOp;
        pxr::SdfTokenListOp::ItemVector schemasList;
        schemasList.push_back(pxr::TfToken("IsaacRtxLidarSensorAPI"));
        schemasListOp.SetAddedItems(schemasList);
        lidarPrim.GetPrim().SetMetadata(pxr::UsdTokens->apiSchemas, pxr::VtValue(schemasListOp));

        pxr::UsdAttribute sensorTypeAttr =
            lidarPrim.GetPrim().CreateAttribute(pxr::TfToken("cameraSensorType"), pxr::SdfValueTypeNames->Token, false);
        sensorTypeAttr.Set(pxr::TfToken("lidar"));

        pxr::TfToken tokenValue1("camera");
        pxr::TfToken tokenValue2("radar");
        pxr::TfToken tokenValue3("lidar");

        pxr::VtArray<pxr::TfToken> validTokens{ tokenValue1, tokenValue2, tokenValue3 };
        // Set the valid tokens for the attribute
        sensorTypeAttr.SetMetadata(pxr::TfToken("allowedTokens"), validTokens);


        lidarPrim.GetPrim()
            .CreateAttribute(pxr::TfToken("sensorModelPluginName"), pxr::SdfValueTypeNames->String, false)
            .Set("omni.sensors.nv.lidar.lidar_core.plugin");
        omni::kit::IApp* app = carb::getCachedInterface<omni::kit::IApp>();
        omni::ext::ExtensionManager* extManager = app->getExtensionManager();
        pxr::SdfLayerRefPtr rootLayer = stage->GetRootLayer();

        // Get the real path of the root layer, and a few extra useful paths
        std::string stagePath = getParent(rootLayer->GetRealPath());
        std::string sensorPath =
            omni::ext::getExtensionPath(extManager, omni::ext::getEnabledExtensionId(extManager, "omni.isaac.sensor"));
        std::string importerPath =
            omni::ext::getExtensionPath(extManager, omni::ext::getEnabledExtensionId(extManager, "omni.importer.urdf"));
        if (lidar.isaacSimConfig != "")
        {
            // If it's a reference for a config file
            if (hasExtension(lidar.isaacSimConfig, "json"))
            {
                std::string configPath = resolveXrefPath(assetRoot_, urdfPath_, lidar.isaacSimConfig);
                std::string conf_name = getPathStem(configPath.c_str());
                if (stagePath != "")
                {
                    omniClientWait(
                        omniClientCopy(configPath.c_str(), pathJoin(stagePath, conf_name + ".json").c_str(), {}, {}));
                    if (sensorPath != "")
                    {
                        createSymbolicLink(
                            pathJoin(stagePath, conf_name + ".json"),
                            pathJoin(pathJoin(pathJoin(sensorPath, "data"), "lidar_configs"), conf_name + ".json"));
                    }
                }
                else
                {
                    CARB_LOG_WARN("Cannot copy/link over lidar configuration when importing into an in-memory stage.");
                }
                lidarPrim.GetPrim()
                    .CreateAttribute(pxr::TfToken("sensorModelConfig"), pxr::SdfValueTypeNames->String, false)
                    .Set(conf_name);
            }
            // Otherwise it's a preset config
            else
            {
                lidarPrim.GetPrim()
                    .CreateAttribute(pxr::TfToken("sensorModelConfig"), pxr::SdfValueTypeNames->String, false)
                    .Set(lidar.isaacSimConfig);
            }
        }
        // If no cofig file is provided, use the default urdf values to create a basic lidar sensor
        else if (stagePath != "")
        {

            std::string template_file =
                pathJoin(pathJoin(pathJoin(importerPath, "data"), "lidar_sensor_template"), "lidar_template.json");
            std::ifstream ifs(template_file);
            if (!ifs.is_open())
            {
                CARB_LOG_ERROR("Failed to open lidar template file: %s", template_file.c_str());
                lidarPrim.GetPrim()
                    .CreateAttribute(pxr::TfToken("sensorModelConfig"), pxr::SdfValueTypeNames->String, false)
                    .Set("");
            }
            else
            {

                std::string jsonStr((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

                // Parse the JSON
                rapidjson::Document doc;
                doc.Parse(jsonStr.c_str());
                rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

                size_t totalRays = lidar.horizontal.samples * lidar.vertical.samples;

                doc["profile"]["scanRateBaseHz"] = lidar.updateRate;
                doc["profile"]["reportRateBaseHz"] = lidar.updateRate;

                doc["profile"]["numberOfEmitters"] = totalRays;
                doc["profile"]["numberOfChannels"] = totalRays;
                doc["profile"]["numLines"] = totalRays;
                float hMinDeg = lidar.horizontal.minAngle * (180.0 / M_PI);
                float vMinDeg = lidar.vertical.minAngle * (180.0 / M_PI);

                float hMaxDeg = lidar.horizontal.maxAngle * (180.0 / M_PI);
                float vMaxDeg = lidar.vertical.maxAngle * (180.0 / M_PI);

                doc["profile"]["startAzimuthDeg"] = hMinDeg;
                doc["profile"]["endAzimuthDeg"] = hMaxDeg;

                doc["profile"]["downElevationDeg"] = vMinDeg;
                doc["profile"]["upElevationDeg"] = vMaxDeg;

                float horizontal_step = ((lidar.horizontal.maxAngle - lidar.horizontal.minAngle) * (180.0 / M_PI)) /
                                        (lidar.horizontal.samples - 1);
                float vertical_step = ((lidar.vertical.maxAngle - lidar.vertical.minAngle) * (180.0 / M_PI)) /
                                      (lidar.horizontal.samples - 1);
                if (lidar.hasHorizontal || lidar.hasVertical)
                {
                    doc["profile"]["emitterStates"][0]["azimuthDeg"].Clear();
                    doc["profile"]["emitterStates"][0]["elevationDeg"].Clear();
                    doc["profile"]["emitterStates"][0]["fireTimeNs"].Clear();
                    doc["profile"]["emitterStates"][0]["channelId"].Clear();
                    doc["profile"]["emitterStates"][0]["rangeId"].Clear();
                    doc["profile"]["emitterStates"][0]["bank"].Clear();
                    doc["profile"]["numRaysPerLine"].Clear();
                    int count = 0;
                    for (size_t vs = 0; vs < lidar.vertical.samples; vs++)
                    {
                        doc["profile"]["numRaysPerLine"] =
                            doc["profile"]["numRaysPerLine"].PushBack(lidar.horizontal.samples, allocator);
                        for (size_t hs = 0; hs < lidar.horizontal.samples; hs++)
                        {
                            doc["profile"]["emitterStates"][0]["azimuthDeg"].PushBack(
                                hMinDeg + hs * horizontal_step, allocator);
                            doc["profile"]["emitterStates"][0]["elevationDeg"].PushBack(
                                vMinDeg + vs * vertical_step, allocator);
                            doc["profile"]["emitterStates"][0]["fireTimeNs"].PushBack(0, allocator);
                            doc["profile"]["emitterStates"][0]["channelId"].PushBack(count++, allocator);
                            doc["profile"]["emitterStates"][0]["rangeId"].PushBack(0, allocator);
                            doc["profile"]["emitterStates"][0]["bank"].PushBack(totalRays - count, allocator);
                        }
                    }
                }
                std::string conf_name = robot.name + "_" + lidar.name;
                saveJsonToFile(doc, pathJoin(stagePath, conf_name + ".json"));
                if (sensorPath != "")
                {
                    createSymbolicLink(
                        pathJoin(stagePath, conf_name + ".json"),
                        pathJoin(pathJoin(pathJoin(sensorPath, "data"), "lidar_configs"), conf_name + ".json"));
                }
                lidarPrim.GetPrim()
                    .CreateAttribute(pxr::TfToken("sensorModelConfig"), pxr::SdfValueTypeNames->String, false)
                    .Set(conf_name);
            }
        }
        else
        {
            CARB_LOG_WARN(
                "Cannot copy/link over lidar configuration when importing into an in-memory stage. Configuration file not generated.");
        }
    }
}

const char* getJointAxis(UrdfAxis axis, Quat& direction)
{
    Vec3 currentAxis(axis.x, axis.y, axis.z);
    // In principle nothing needs to be done if joint is aligned with an axis
    // If joint is inverse-aligned with axis (e.g. (-1,0,0)), we need to provide a rotation direction so that the
    // Joint rotation is applied at 180 degrees from the parent/child bodies.

    // If Joint axis is not aligned with a principal axis (unlikely to happen), there's another portion of the code
    // handling this orientation change
    direction = Quat(0, 0, 0, 1);
    if (abs(Dot(currentAxis, Vec3(1.0f, 0.0f, 0.0f))) > 1 - kSmallEps)
    {
        if (Dot(currentAxis, Vec3(1.0f, 0.0f, 0.0f)) < 0)
        {
            direction = Quat(0, 0, 1, 0);
        }
        return "X";
    }
    else if (abs(Dot(currentAxis, Vec3(0.0f, 1.0f, 0.0f))) > 1 - kSmallEps)
    {
        if (Dot(currentAxis, Vec3(0.0f, 1.0f, 0.0f)) < 0)
        {
            direction = Quat(1, 0, 0, 0);
        }
        return "Y";
    }
    else if (abs(Dot(currentAxis, Vec3(0.0f, 0.0f, 1.0f))) > 1 - kSmallEps)
    {
        if (Dot(currentAxis, Vec3(0.0f, 0.0f, 1.0f)) < 0)
        {
            direction = Quat(0, 1, 0, 0);
        }
        return "Z";
    }
    return "";
}


template <class T>
void AddSingleJoint(const UrdfJoint& joint,
                    pxr::UsdStageWeakPtr stage,
                    const pxr::SdfPath& jointPath,
                    pxr::UsdPhysicsJoint& jointPrimBase,
                    const float distanceScale,
                    const ImportConfig& config)
{

    T jointPrim = T::Define(stage, pxr::SdfPath(jointPath));
    jointPrimBase = jointPrim;
    Quat direction;
    const char* axis = getJointAxis(joint.axis, direction);
    if (axis != "")
    {
        jointPrim.CreateAxisAttr().Set(pxr::TfToken(axis));
    }
    else
    {
        jointPrim.CreateAxisAttr().Set(pxr::TfToken("X"));
    }

    // Set the limits if the joint is anything except a continuous joint
    if (joint.type != UrdfJointType::CONTINUOUS)
    {
        // Angular limits are in degrees so scale accordingly
        float scale = 180.0f / static_cast<float>(M_PI);
        if (joint.type == UrdfJointType::PRISMATIC)
        {
            scale = distanceScale;
        }
        jointPrim.CreateLowerLimitAttr().Set(scale * joint.limit.lower);
        jointPrim.CreateUpperLimitAttr().Set(scale * joint.limit.upper);
    }
    pxr::PhysxSchemaPhysxJointAPI physxJoint = pxr::PhysxSchemaPhysxJointAPI::Apply(jointPrim.GetPrim());
    if (joint.dynamics.friction > 0.0f)
    {
        physxJoint.CreateJointFrictionAttr().Set(joint.dynamics.friction);
    }

    if (joint.type == UrdfJointType::PRISMATIC)
    {
        pxr::PhysxSchemaJointStateAPI::Apply(jointPrim.GetPrim(), pxr::TfToken("linear"));
        if (joint.mimic.joint == "" || !config.parseMimic)
        {
            pxr::UsdPhysicsDriveAPI driveAPI =
                pxr::UsdPhysicsDriveAPI::Apply(jointPrim.GetPrim(), pxr::TfToken("linear"));
            // convert kg*m/s^2 to kg * cm /s^2
            driveAPI.CreateMaxForceAttr().Set(joint.limit.effort > 0.0f ? joint.limit.effort * distanceScale : FLT_MAX);

            // change drive type
            if (joint.drive.driveType == UrdfJointDriveType::FORCE)
            {
                driveAPI.CreateTypeAttr().Set(pxr::TfToken("force"));
            }
            else
            {
                driveAPI.CreateTypeAttr().Set(pxr::TfToken("acceleration"));
            }

            // change drive target type
            if (joint.drive.targetType == UrdfJointTargetType::POSITION)
            {
                driveAPI.CreateTargetPositionAttr().Set(joint.drive.target);
            }
            else if (joint.drive.targetType == UrdfJointTargetType::VELOCITY)
            {
                driveAPI.CreateTargetVelocityAttr().Set(joint.drive.target);
            }

            // change drive stiffness and damping
            if (joint.drive.targetType != UrdfJointTargetType::NONE)
            {
                if (joint.drive.targetType == UrdfJointTargetType::POSITION)
                {
                    driveAPI.CreateStiffnessAttr().Set(joint.drive.strength);
                    driveAPI.CreateDampingAttr().Set(joint.drive.damping);
                }
                if (joint.drive.targetType == UrdfJointTargetType::VELOCITY)
                {
                    driveAPI.CreateStiffnessAttr().Set(0.0f);
                    driveAPI.CreateDampingAttr().Set(joint.drive.strength);
                }
            }
            else
            {
                driveAPI.CreateDampingAttr().Set(joint.dynamics.damping);
                driveAPI.CreateStiffnessAttr().Set(joint.dynamics.stiffness);
            }
        }

        // Prismatic joint velocity should be scaled to stage units, but not revolute
        physxJoint.CreateMaxJointVelocityAttr().Set(
            joint.limit.velocity > 0.0f ? static_cast<float>(joint.limit.velocity * distanceScale) : FLT_MAX);
    }
    // continuous and revolute are identical except for setting limits
    else if (joint.type == UrdfJointType::REVOLUTE || joint.type == UrdfJointType::CONTINUOUS)
    {
        pxr::PhysxSchemaJointStateAPI::Apply(jointPrim.GetPrim(), pxr::TfToken("angular"));

        if (joint.mimic.joint == "" || !config.parseMimic)
        {
            pxr::UsdPhysicsDriveAPI driveAPI =
                pxr::UsdPhysicsDriveAPI::Apply(jointPrim.GetPrim(), pxr::TfToken("angular"));
            // convert kg*m/s^2 * m to kg * cm /s^2 * cm
            driveAPI.CreateMaxForceAttr().Set(
                joint.limit.effort > 0.0f ? joint.limit.effort * distanceScale * distanceScale : FLT_MAX);

            // change drive type
            if (joint.drive.driveType == UrdfJointDriveType::FORCE)
            {
                driveAPI.CreateTypeAttr().Set(pxr::TfToken("force"));
            }
            else
            {
                driveAPI.CreateTypeAttr().Set(pxr::TfToken("acceleration"));
            }

            // change drive target type
            if (joint.drive.targetType == UrdfJointTargetType::POSITION)
            {
                driveAPI.CreateTargetPositionAttr().Set(joint.drive.target);
            }
            else if (joint.drive.targetType == UrdfJointTargetType::VELOCITY)
            {
                driveAPI.CreateTargetVelocityAttr().Set(joint.drive.target);
            }

            // change drive stiffness and damping
            if (joint.drive.targetType != UrdfJointTargetType::NONE)
            {
                if (joint.drive.targetType == UrdfJointTargetType::POSITION)
                {
                    driveAPI.CreateStiffnessAttr().Set(joint.drive.strength);
                    driveAPI.CreateDampingAttr().Set(joint.drive.damping);
                }
                if (joint.drive.targetType == UrdfJointTargetType::VELOCITY)
                {
                    driveAPI.CreateStiffnessAttr().Set(0.0f);
                    driveAPI.CreateDampingAttr().Set(joint.drive.strength);
                }
            }
            else
            {
                driveAPI.CreateDampingAttr().Set(joint.dynamics.damping);
                driveAPI.CreateStiffnessAttr().Set(joint.dynamics.stiffness);
            }
        }
        // Convert revolute joint velocity limit to deg/s
        physxJoint.CreateMaxJointVelocityAttr().Set(
            joint.limit.velocity > 0.0f ? static_cast<float>(180.0f / M_PI * joint.limit.velocity) : FLT_MAX);
    }

    if (config.parseMimic)
    {
        if (joint.mimic.joint != "")
        {
            auto axisToken = pxr::UsdPhysicsTokens.Get()->rotX;
            Quat direction;
            const char* axis = getJointAxis(joint.axis, direction);
            {
                if (axis == "Y")
                {
                    axisToken = pxr::UsdPhysicsTokens.Get()->rotY;
                }
                else if (axis == "Z")
                {
                    axisToken = pxr::UsdPhysicsTokens.Get()->rotZ;
                }
            }

            auto mimicAPI = pxr::PhysxSchemaPhysxMimicJointAPI::Apply(jointPrim.GetPrim(), axisToken);
            // Gearing is the opposite direction from mimic multiplier
            mimicAPI.GetGearingAttr().Set(-joint.mimic.multiplier);

            mimicAPI.GetOffsetAttr().Set(joint.mimic.offset);

            // Find source joint
            auto source_prim = FindPrimByNameAndType(stage, joint.mimic.joint, pxr::TfType::Find<pxr::UsdPhysicsJoint>());
            if (source_prim)
            {
                mimicAPI.GetReferenceJointRel().AddTarget(source_prim.GetPath());
            }
        }
    }
}


void UrdfImporter::addJoint(pxr::UsdStageWeakPtr stage,
                            pxr::UsdGeomXform robotPrim,
                            const UrdfJoint& joint,
                            const Transform& poseJointToParentBody)
{

    std::string parentLinkPath = robotPrim.GetPath().GetString() + "/" + joint.parentLinkName;
    std::string childLinkPath = robotPrim.GetPath().GetString() + "/" + joint.childLinkName;
    std::string jointPath = parentLinkPath + "/" + joint.name;

    if (!pxr::SdfPath::IsValidPathString(jointPath))
    {
        // jn->getName starts with a number which is not valid for usd path, so prefix it with "joint"
        jointPath = parentLinkPath + "/joint" + joint.name;
    }
    pxr::UsdPhysicsJoint jointPrim;
    if (joint.type == UrdfJointType::FIXED)
    {
        jointPrim = pxr::UsdPhysicsFixedJoint::Define(stage, pxr::SdfPath(jointPath));
    }
    else if (joint.type == UrdfJointType::PRISMATIC)
    {
        AddSingleJoint<pxr::UsdPhysicsPrismaticJoint>(
            joint, stage, pxr::SdfPath(jointPath), jointPrim, float(config.distanceScale), config);
    }
    // else if (joint.type == UrdfJointType::SPHERICAL)
    // {
    //     AddSingleJoint<PhysicsSchemaSphericalJoint>(jn, stage, SdfPath(jointPath), jointPrim, skel,
    //     distanceScale);
    // }
    else if (joint.type == UrdfJointType::REVOLUTE || joint.type == UrdfJointType::CONTINUOUS)
    {
        AddSingleJoint<pxr::UsdPhysicsRevoluteJoint>(
            joint, stage, pxr::SdfPath(jointPath), jointPrim, float(config.distanceScale), config);
    }
    else if (joint.type == UrdfJointType::FLOATING)
    {
        // There is no joint, skip
        return;
    }


    pxr::SdfPathVector val0{ pxr::SdfPath(parentLinkPath) };
    pxr::SdfPathVector val1{ pxr::SdfPath(childLinkPath) };

    if (parentLinkPath != "")
    {
        jointPrim.CreateBody0Rel().SetTargets(val0);
    }

    pxr::GfVec3f localPos0 = config.distanceScale * pxr::GfVec3f(poseJointToParentBody.p.x, poseJointToParentBody.p.y,
                                                                 poseJointToParentBody.p.z);
    pxr::GfQuatf localRot0 = pxr::GfQuatf(
        poseJointToParentBody.q.w, poseJointToParentBody.q.x, poseJointToParentBody.q.y, poseJointToParentBody.q.z);
    pxr::GfVec3f localPos1 = config.distanceScale * pxr::GfVec3f(0, 0, 0);
    pxr::GfQuatf localRot1 = pxr::GfQuatf(1, 0, 0, 0);

    Quat jointAxisRotQuat(0.0f, 0.0f, 0.0f, 1.0f);
    const char* axis = getJointAxis(joint.axis, jointAxisRotQuat);
    if (axis == "")
    {
        CARB_LOG_WARN(
            "%s: Joint Axis is not body aligned with X, Y or Z primary axis. Adjusting PhysX joint alignment to Axis X and reorienting bodies.",
            joint.name.c_str());
        // If the joint axis is not aligned with X, Y or Z we revert back to PhysX's default of aligning to X axis.
        // Need to rotate the joint frame to match the urdf defined axis
        // convert joint axis to angle-axis representation
        Vec3 jointAxisRotAxis = -Cross(urdfAxisToVec(joint.axis), Vec3(1.0f, 0.0f, 0.0f));
        float jointAxisRotAngle = acos(joint.axis.x); // this is equal to acos(Dot(joint.axis, Vec3(1.0f, 0.0f, 0.0f)))
        if (Dot(jointAxisRotAxis, jointAxisRotAxis) < kSmallEps)
        {
            // for axis along x we define an arbitrary perpendicular rotAxis (along y).
            // In that case the angle is 0 or 180deg
            jointAxisRotAxis = Vec3(0.0f, 1.0f, 0.0f);
        }
        // normalize jointAxisRotAxis
        jointAxisRotAxis /= sqrtf(Dot(jointAxisRotAxis, jointAxisRotAxis));
        // rotate the parent frame by the axis

        jointAxisRotQuat = QuatFromAxisAngle(jointAxisRotAxis, jointAxisRotAngle);
    }

    // apply transforms
    jointPrim.CreateLocalPos0Attr().Set(localPos0);
    jointPrim.CreateLocalRot0Attr().Set(
        localRot0 * pxr::GfQuatf(jointAxisRotQuat.w, jointAxisRotQuat.x, jointAxisRotQuat.y, jointAxisRotQuat.z));

    if (childLinkPath != "")
    {
        jointPrim.CreateBody1Rel().SetTargets(val1);
    }
    jointPrim.CreateLocalPos1Attr().Set(localPos1);
    jointPrim.CreateLocalRot1Attr().Set(
        localRot1 * pxr::GfQuatf(jointAxisRotQuat.w, jointAxisRotQuat.x, jointAxisRotQuat.y, jointAxisRotQuat.z));

    jointPrim.CreateBreakForceAttr().Set(FLT_MAX);
    jointPrim.CreateBreakTorqueAttr().Set(FLT_MAX);

    // TODO: FIx?
    // auto linkAPI = pxr::UsdPhysicsJoint::Apply(stage->GetPrimAtPath(pxr::SdfPath(jointPath)));
    // linkAPI.CreateArticulationTypeAttr().Set(pxr::TfToken("articulatedJoint"));
}
void UrdfImporter::addLinksAndJoints(pxr::UsdStageWeakPtr stage,
                                     const Transform& poseParentToWorld,
                                     const KinematicChain::Node* parentNode,
                                     const UrdfRobot& robot,
                                     pxr::UsdGeomXform robotPrim)
{

    // Create root joint only once
    if (parentNode->parentJointName_ == "")
    {
        const UrdfLink& urdfLink = robot.links.at(parentNode->linkName_);
        addRigidBody(stage, urdfLink, poseParentToWorld, robotPrim, robot);
        if (config.fixBase)
        {
            std::string rootJointPath = robotPrim.GetPath().GetString() + "/root_joint";
            pxr::UsdPhysicsFixedJoint rootJoint = pxr::UsdPhysicsFixedJoint::Define(stage, pxr::SdfPath(rootJointPath));
            pxr::UsdPrim rootLinkPrim = rootJoint.GetPrim();
            // Apply articulation root schema
            pxr::UsdPhysicsArticulationRootAPI physicsSchema = pxr::UsdPhysicsArticulationRootAPI::Apply(rootLinkPrim);
            pxr::PhysxSchemaPhysxArticulationAPI physxSchema = pxr::PhysxSchemaPhysxArticulationAPI::Apply(rootLinkPrim);
            // Set schema attributes
            physxSchema.CreateEnabledSelfCollisionsAttr().Set(config.selfCollision);
            // These are reasonable defaults, might want to expose them via the import config in the future.
            physxSchema.CreateSolverPositionIterationCountAttr().Set(32);
            physxSchema.CreateSolverVelocityIterationCountAttr().Set(1);
            pxr::SdfPathVector val1{ pxr::SdfPath(robotPrim.GetPath().GetString() + "/" + urdfLink.name) };
            if (!urdfLink.inertial.hasMass)
            {
                auto linkPrim = stage->GetPrimAtPath(pxr::SdfPath(robotPrim.GetPath().GetString() + "/" + urdfLink.name));
                pxr::UsdPhysicsMassAPI massAPI = pxr::UsdPhysicsMassAPI(linkPrim);
                // set base mass to zero and let physx deal with that to provide a mass that stabilizes
                // the base of the articulation
                massAPI.GetMassAttr().Set(0.0f);
            }
            rootJoint.CreateBody1Rel().SetTargets(val1);
        }
    }

    if (!parentNode->childNodes_.empty())
    {
        for (const auto& childNode : parentNode->childNodes_)
        {
            if (robot.joints.find(childNode->parentJointName_) != robot.joints.end())
            {
                if (robot.links.find(childNode->linkName_) != robot.links.end())
                {
                    const UrdfJoint urdfJoint = robot.joints.at(childNode->parentJointName_);
                    const UrdfLink& childLink = robot.links.at(childNode->linkName_);
                    // const UrdfLink& parentLink = robot.links.at(parentNode->linkName_);

                    Transform poseJointToLink = urdfJoint.origin;
                    // According to URDF spec, the frame of a link coincides with its parent joint frame
                    Transform poseLinkToWorld = poseParentToWorld * poseJointToLink;
                    // if (!parentLink.softs.size() && !childLink.softs.size()) // rigid parent, rigid child
                    {
                        addRigidBody(stage, childLink, poseLinkToWorld, robotPrim, robot);
                        addJoint(stage, robotPrim, urdfJoint, poseJointToLink);

                        // RigidBodyTopo bodyTopo;
                        // bodyTopo.bodyIndex = asset->bodyLookup.at(childNode->linkName_);
                        // bodyTopo.parentIndex = asset->bodyLookup.at(parentNode->linkName_);
                        // bodyTopo.jointIndex = asset->jointLookup.at(childNode->parentJointName_);
                        // bodyTopo.jointSpecStart = asset->jointLookup.at(childNode->parentJointName_);
                        // // URDF only has 1 DOF joints
                        // bodyTopo.jointSpecCount = 1;
                        // asset->rigidBodyHierarchy.push_back(bodyTopo);
                    }

                    // Recurse through the links children
                    addLinksAndJoints(stage, poseLinkToWorld, childNode.get(), robot, robotPrim);
                }
                else
                {
                    CARB_LOG_ERROR("Failed to Create Joint <%s>: Child link <%s> not found",
                                   childNode->parentJointName_.c_str(), childNode->linkName_.c_str());
                }
            }
            else
            {
                CARB_LOG_WARN("Joint <%s> is undefined", childNode->parentJointName_.c_str());
            }
        }
    }
}

void UrdfImporter::addMaterials(pxr::UsdStageWeakPtr stage, const UrdfRobot& robot, const pxr::SdfPath& prefixPath)
{
    stage->DefinePrim(pxr::SdfPath(prefixPath.GetString() + "/Looks"), pxr::TfToken("Scope"));
    for (auto& mat : robot.materials)
    {
        addMaterial(stage, mat, prefixPath);
    }
}

pxr::UsdShadeMaterial UrdfImporter::addMaterial(pxr::UsdStageWeakPtr stage,
                                                const std::pair<std::string, UrdfMaterial>& mat,
                                                const pxr::SdfPath& prefixPath)
{
    auto& color = mat.second.color;
    std::string name = mat.second.name;
    if (name == "")
    {
        name = mat.first;
    }
    if (color.r >= 0 && color.g >= 0 && color.b >= 0)
    {
        pxr::SdfPath shaderPath =
            prefixPath.AppendPath(pxr::SdfPath("Looks/" + makeValidUSDIdentifier("material_" + name)));

        pxr::UsdShadeMaterial matPrim = pxr::UsdShadeMaterial::Define(stage, shaderPath);
        if (matPrim)
        {
            pxr::UsdShadeShader pbrShader =
                pxr::UsdShadeShader::Define(stage, shaderPath.AppendPath(pxr::SdfPath("Shader")));
            if (pbrShader)
            {
                auto shader_out = pbrShader.CreateOutput(pxr::TfToken("out"), pxr::SdfValueTypeNames->Token);
                matPrim.CreateSurfaceOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                matPrim.CreateVolumeOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                matPrim.CreateDisplacementOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                pbrShader.GetImplementationSourceAttr().Set(pxr::UsdShadeTokens->sourceAsset);
                pbrShader.SetSourceAsset(pxr::SdfAssetPath("OmniPBR.mdl"), pxr::TfToken("mdl"));
                pbrShader.SetSourceAssetSubIdentifier(pxr::TfToken("OmniPBR"), pxr::TfToken("mdl"));

                pbrShader.CreateInput(pxr::TfToken("diffuse_color_constant"), pxr::SdfValueTypeNames->Color3f)
                    .Set(pxr::GfVec3f(color.r, color.g, color.b));

                matPrimPaths[name] = shaderPath.GetString();
                return matPrim;
            }
            else
            {
                CARB_LOG_WARN("Couldn't create shader at: %s", shaderPath.GetString().c_str());
            }
        }
        else
        {
            CARB_LOG_WARN("Couldn't create material at: %s", shaderPath.GetString().c_str());
        }
    }
    return pxr::UsdShadeMaterial();
}

std::string UrdfImporter::addToStage(pxr::UsdStageWeakPtr stage,
                                     const UrdfRobot& urdfRobot,
                                     bool getArticulationRoot = false)
{
    if (urdfRobot.links.size() == 0)
    {
        CARB_LOG_WARN("Cannot add robot to stage, number of links is zero");
        return "";
    }
    // The limit for links is now a 32bit index so this shouldn't be needed anymore
    // if (urdfRobot.links.size() >= 64)
    // {
    //     CARB_LOG_WARN(
    //         "URDF cannot have more than 63 links to be imported as a physx articulation. Try enabling the merge fixed
    //         joints option to reduce the number of links.");
    //     CARB_LOG_WARN("URDF has %d links", static_cast<int>(urdfRobot.links.size()));
    //     return "";
    // }

    if (config.createPhysicsScene)
    {
        bool sceneExists = false;
        pxr::UsdPrimRange range = stage->Traverse();
        for (pxr::UsdPrimRange::iterator iter = range.begin(); iter != range.end(); ++iter)
        {
            pxr::UsdPrim prim = *iter;

            if (prim.IsA<pxr::UsdPhysicsScene>())
            {
                sceneExists = true;
            }
        }
        if (!sceneExists)
        {
            // Create physics scene
            pxr::UsdPhysicsScene scene = pxr::UsdPhysicsScene::Define(stage, pxr::SdfPath("/physicsScene"));
            scene.CreateGravityDirectionAttr().Set(pxr::GfVec3f(0.0f, 0.0f, -1.0));
            scene.CreateGravityMagnitudeAttr().Set(9.81f * config.distanceScale);

            pxr::PhysxSchemaPhysxSceneAPI physxSceneAPI =
                pxr::PhysxSchemaPhysxSceneAPI::Apply(stage->GetPrimAtPath(pxr::SdfPath("/physicsScene")));
            physxSceneAPI.CreateEnableCCDAttr().Set(true);
            physxSceneAPI.CreateEnableStabilizationAttr().Set(true);
            physxSceneAPI.CreateEnableGPUDynamicsAttr().Set(false);

            physxSceneAPI.CreateBroadphaseTypeAttr().Set(pxr::TfToken("MBP"));
            physxSceneAPI.CreateSolverTypeAttr().Set(pxr::TfToken("TGS"));
        }
    }

    pxr::SdfPath primPath =
        pxr::SdfPath(GetNewSdfPathString(stage, stage->GetDefaultPrim().GetPath().GetString() + "/" +
                                                    makeValidUSDIdentifier(std::string(urdfRobot.name))));
    if (config.makeDefaultPrim)
        primPath = pxr::SdfPath(GetNewSdfPathString(stage, "/" + makeValidUSDIdentifier(std::string(urdfRobot.name))));
    // // Remove the prim we are about to add in case it exists
    // if (stage->GetPrimAtPath(primPath))
    // {
    //     stage->RemovePrim(primPath);
    // }

    pxr::UsdGeomXform robotPrim = pxr::UsdGeomXform::Define(stage, primPath);

    pxr::UsdGeomXformable gprim = pxr::UsdGeomXformable(robotPrim);
    gprim.ClearXformOpOrder();
    gprim.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(0, 0, 0));
    gprim.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfQuatd(1, 0, 0, 0));
    gprim.AddScaleOp(pxr::UsdGeomXformOp::PrecisionDouble).Set(pxr::GfVec3d(1, 1, 1));

    if (config.makeDefaultPrim)
    {
        stage->SetDefaultPrim(robotPrim.GetPrim());
    }


    KinematicChain chain;
    if (!chain.computeKinematicChain(urdfRobot))
    {
        return "";
    }

    if (!config.makeInstanceable)
    {
        addMaterials(stage, urdfRobot, primPath);
    }
    else
    {
        // first create instanceable meshes USD
        std::string instanceableStagePath = config.instanceableMeshUsdPath;
        if (config.instanceableMeshUsdPath[0] == '.')
        {
            // make relative path relative to output directory
            std::string relativePath = config.instanceableMeshUsdPath.substr(1);
            std::string curStagePath = stage->GetRootLayer()->GetIdentifier();
            std::string directory;
            size_t pos = curStagePath.find_last_of("\\/");
            directory = (std::string::npos == pos) ? "" : curStagePath.substr(0, pos);
            instanceableStagePath = directory + relativePath;
        }

        pxr::UsdStageRefPtr instanceableMeshStage;
        instanceableMeshStage = pxr::UsdStage::Open(instanceableStagePath);
        if (!instanceableMeshStage)
        {
            CARB_LOG_INFO("Creating Stage: %s", instanceableStagePath.c_str());
            instanceableMeshStage = pxr::UsdStage::CreateNew(instanceableStagePath);
        }
        else
        {
            for (const auto& p : instanceableMeshStage->GetPrimAtPath(pxr::SdfPath("/")).GetChildren())
            {
                instanceableMeshStage->RemovePrim(p.GetPath());
            }
            instanceableMeshStage->Save();
        }
        std::string robotBasePath = robotPrim.GetPath().GetString() + "/";
        buildInstanceableStage(instanceableMeshStage, chain.baseNode.get(), robotBasePath, urdfRobot);
        instanceableMeshStage->Export(instanceableStagePath);
    }

    addLinksAndJoints(stage, Transform(), chain.baseNode.get(), urdfRobot, robotPrim);

    // Add articulation root prim on the actual root link
    pxr::SdfPath rootLinkPrimPath = pxr::SdfPath(primPath.GetString() + "/root_joint");
    if (!config.fixBase)
    {
        rootLinkPrimPath = pxr::SdfPath(primPath.GetString() + "/" + urdfRobot.rootLink);
        pxr::UsdPrim rootLinkPrim = stage->GetPrimAtPath(rootLinkPrimPath);
        // Apply articulation root schema
        pxr::UsdPhysicsArticulationRootAPI physicsSchema = pxr::UsdPhysicsArticulationRootAPI::Apply(rootLinkPrim);
        pxr::PhysxSchemaPhysxArticulationAPI physxSchema = pxr::PhysxSchemaPhysxArticulationAPI::Apply(rootLinkPrim);
        // Set schema attributes
        physxSchema.CreateEnabledSelfCollisionsAttr().Set(config.selfCollision);
        // These are reasonable defaults, might want to expose them via the import config in the future.
        physxSchema.CreateSolverPositionIterationCountAttr().Set(32);
        physxSchema.CreateSolverVelocityIterationCountAttr().Set(1);
    }

    if (getArticulationRoot)
    {
        return rootLinkPrimPath.GetString();
    }
    else
    {
        return primPath.GetString();
    }
}
}
}
}
