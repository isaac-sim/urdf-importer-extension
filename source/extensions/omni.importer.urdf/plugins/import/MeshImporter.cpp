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

#include "MeshImporter.h"

#include "../core/PathUtils.h"
#include "ImportHelpers.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include <carb/logging/Log.h>


#if __has_include(<filesystem>)
#    include <filesystem>
#elif __has_include(<experimental/filesystem>)
#    include <experimental/filesystem>
#else
error "Missing the <filesystem> header."
#endif

#include "../utils/Path.h"

#include <OmniClient.h>
#include <cmath>
#include <set>
#include <stack>
#include <unordered_set>


namespace omni
{
namespace importer
{
namespace urdf
{

using namespace omni::importer::utils::path;

const static size_t INVALID_MATERIAL_INDEX = SIZE_MAX;

struct ImportTransform
{
    pxr::GfMatrix4d matrix;
    pxr::GfVec3f translation;
    pxr::GfVec3f eulerAngles; // XYZ order
    pxr::GfVec3f scale;
};


struct MeshGeomSubset
{
    pxr::VtArray<int> faceIndices;
    size_t materialIndex = INVALID_MATERIAL_INDEX;
};


struct Mesh
{
    std::string name;
    pxr::VtArray<pxr::GfVec3f> points;
    pxr::VtArray<int> faceVertexCounts;
    pxr::VtArray<int> faceVertexIndices;
    pxr::VtArray<pxr::GfVec3f> normals; // Face varing normals
    pxr::VtArray<pxr::VtArray<pxr::GfVec2f>> uvs; // Face varing uvs
    pxr::VtArray<pxr::VtArray<pxr::GfVec3f>> colors; // Face varing colors
    std::vector<MeshGeomSubset> meshSubsets;
};

// static pxr::GfMatrix4d AiMatrixToGfMatrix(const aiMatrix4x4& matrix)
// {
//     return pxr::GfMatrix4d(matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2,
//                            matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4);
// }

static pxr::GfVec3f AiVector3dToGfVector3f(const aiVector3D& vector)
{
    return pxr::GfVec3f(vector.x, vector.y, vector.z);
}

static pxr::GfVec2f AiVector3dToGfVector2f(const aiVector3D& vector)
{
    return pxr::GfVec2f(vector.x, vector.y);
}

// static pxr::GfVec3h AiVector3dToGfVector3h(const aiVector3D& vector)
// {
//     return pxr::GfVec3h(vector.x, vector.y, vector.z);
// }

// static pxr::GfQuatf AiQuatToGfVector(const aiQuaternion& quat)
// {
//     return pxr::GfQuatf(quat.w, quat.x, quat.y, quat.z);
// }

// static pxr::GfQuath AiQuatToGfVectorh(const aiQuaternion& quat)
// {
//     return pxr::GfQuath(quat.w, quat.x, quat.y, quat.z);
// }

// static pxr::GfVec3f AiColor3DToGfVector3f(const aiColor3D& color)
// {
//     return pxr::GfVec3f(color.r, color.g, color.b);
// }


// static ImportTransform AiMatrixToTransform(const aiMatrix4x4& matrix)
// {
//     ImportTransform transform;
//     transform.matrix =
//         pxr::GfMatrix4d(matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2,
//                         matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4);

//     aiVector3D translation, rotation, scale;
//     matrix.Decompose(scale, rotation, translation);
//     transform.translation = AiVector3dToGfVector3f(translation);
//     transform.eulerAngles = AiVector3dToGfVector3f(
//         aiVector3D(AI_RAD_TO_DEG(rotation.x), AI_RAD_TO_DEG(rotation.y), AI_RAD_TO_DEG(rotation.z)));
//     transform.scale = AiVector3dToGfVector3f(scale);

//     return transform;
// }

pxr::GfVec3f AiColor4DToGfVector3f(const aiColor4D& color)
{
    return pxr::GfVec3f(color.r, color.g, color.b);
}

struct MeshMaterial
{
    std::string name;
    std::string texturePaths[5];
    bool has_diffuse;
    aiColor3D diffuse;
    bool has_emissive;
    aiColor3D emissive;
    bool has_metallic;
    float metallic{ 0 };
    bool has_specular;
    float specular{ 0 };

    aiTextureType textures[5] = { aiTextureType_DIFFUSE, aiTextureType_HEIGHT, aiTextureType_REFLECTION,
                                  aiTextureType_EMISSIVE, aiTextureType_SHININESS };
    const char* props[5] = {
        "diffuse_texture",       "normalmap_texture",           "metallic_texture",
        "emissive_mask_texture", "reflectionroughness_texture",
    };
    MeshMaterial(aiMaterial* m)
    {
        name = std::string(m->GetName().C_Str());
        std::array<aiTextureMapMode, 2> modes;
        aiString path;
        for (int i = 0; i < 5; i++)
        {

            if (m->GetTexture(textures[i], 0, &path, nullptr, nullptr, nullptr, nullptr, modes.data()) == aiReturn_SUCCESS)
            {
                texturePaths[i] = std::string(path.C_Str());
            }
        }
        has_diffuse = (m->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == aiReturn_SUCCESS);

        has_metallic = (m->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS);

        has_specular = (m->Get(AI_MATKEY_SPECULAR_FACTOR, specular) == aiReturn_SUCCESS);

        has_emissive = (m->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == aiReturn_SUCCESS);
    }

    std::string get_hash()
    {
        std::ostringstream ss;
        ss << name;
        for (int i = 0; i < 5; i++)
        {
            if (texturePaths[i] != "")
            {
                ss << std::string(props[i]) + texturePaths[i];
            }
        }
        ss << std::string("D") << diffuse.r << diffuse.g << diffuse.b;
        ss << std::string("M") << metallic;
        ss << std::string("S") << specular;
        ss << std::string("E") << emissive.r << emissive.g << emissive.g;
        return ss.str();
    }
};

std::string ReplaceBackwardSlash(std::string in)
{
    for (auto& c : in)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    return in;
}
static aiMatrix4x4 GetLocalTransform(const aiNode* node)
{
    aiMatrix4x4 transform = node->mTransformation;
    auto parent = node->mParent;
    while (parent)
    {
        std::string name = parent->mName.data;
        // only take scale from root transform, if the parent has a parent then its not a root node
        if (parent->mParent)
        {
            // parent has a parent, not a root note, use full transform
            transform = parent->mTransformation * transform;
            parent = parent->mParent;
        }
        else
        {
            // this is a root node, only take scale
            aiVector3D pos, scale;
            aiQuaternion rot;
            parent->mTransformation.Decompose(scale, rot, pos);

            aiMatrix4x4 scale_mat;
            transform = aiMatrix4x4::Scaling(scale, scale_mat) * transform;

            break;
        }
    }
    return transform;
}

std::string copyTexture(std::string usdStageIdentifier, std::string texturePath)
{
    // switch any windows-style path into linux backwards slash (omniclient handles windows paths)
    usdStageIdentifier = ReplaceBackwardSlash(usdStageIdentifier);
    texturePath = ReplaceBackwardSlash(texturePath);

    // Assumes the folder structure has already been created.
    int path_idx = (int)usdStageIdentifier.rfind('/');
    std::string parent_folder = usdStageIdentifier.substr(0, path_idx);
    int basename_idx = (int)texturePath.rfind('/');
    std::string textureName = texturePath.substr(basename_idx + 1);
    std::string out = (parent_folder + "/materials/" + textureName);
    omniClientWait(omniClientCopy(texturePath.c_str(), out.c_str(), {}, {}));
    return out;
}

pxr::SdfPath SimpleImport(pxr::UsdStageRefPtr usdStage,
                          std::string path,
                          const aiScene* mScene,
                          const std::string meshPath,
                          std::map<pxr::TfToken, std::string>& materialsList,
                          const bool loadMaterials,
                          const bool flipVisuals,
                          const char* subdivisionScheme,
                          const bool instanceable)
{
    std::vector<Mesh> mMeshPrims;
    std::vector<aiNode*> nodesToProcess;
    std::vector<std::pair<int, aiMatrix4x4>> meshTransforms;
    // Traverse tree and get all of the meshes and the full transform for that node
    nodesToProcess.push_back(mScene->mRootNode);
    std::string mesh_path = ReplaceBackwardSlash(meshPath);
    int basename_idx = (int)mesh_path.rfind('/');
    std::string base_path = mesh_path.substr(0, basename_idx);
    while (nodesToProcess.size() > 0)
    {
        // remove the node
        aiNode* node = nodesToProcess.back();
        if (!node)
        {
            // printf("INVALID NODE\n");
            continue;
        }
        nodesToProcess.pop_back();
        aiMatrix4x4 transform = GetLocalTransform(node);
        for (size_t i = 0; i < node->mNumMeshes; i++)
        {
            // if (flipVisuals)
            // {
            //     aiMatrix4x4 flip;
            //     flip[0][0] = 1.0;
            //     flip[2][1] = 1.0;
            //     flip[1][2] = -1.0;
            //     flip[3][3] = 1.0f;
            //     transform = transform * flip;
            // }
            meshTransforms.push_back(std::pair<int, aiMatrix4x4>(node->mMeshes[i], transform));
        }
        // process any meshes in this node:
        for (size_t i = 0; i < node->mNumChildren; i++)
        {
            nodesToProcess.push_back(node->mChildren[i]);
        }
    }
    // printf("%s TOTAL MESHES: %d\n", path.c_str(), meshTransforms.size());
    mMeshPrims.resize(meshTransforms.size());


    // for (size_t i = 0; i < mScene->mNumMaterials; i++)
    // {
    //     auto material = mScene->mMaterials[i];
    //     // printf("AA %d %s \n", i, material->GetName().C_Str());
    // }

    for (size_t i = 0; i < meshTransforms.size(); i++)
    {
        auto transformedMesh = meshTransforms[i];
        auto assimpMesh = mScene->mMeshes[transformedMesh.first];
        // printf("material index: %d \n", assimpMesh->mMaterialIndex);
        // Gather all mesh points information to sort
        std::vector<Mesh> meshImported;

        for (size_t j = 0; j < assimpMesh->mNumVertices; j++)
        {
            auto vertex = assimpMesh->mVertices[j];
            vertex *= transformedMesh.second;
            mMeshPrims[i].points.push_back(AiVector3dToGfVector3f(vertex));
        }
        for (size_t j = 0; j < assimpMesh->mNumFaces; j++)
        {
            const aiFace& face = assimpMesh->mFaces[j];
            if (face.mNumIndices >= 3)
            {
                for (size_t k = 0; k < face.mNumIndices; k++)
                {
                    mMeshPrims[i].faceVertexIndices.push_back(face.mIndices[k]);
                }
            }
        }
        mMeshPrims[i].uvs.resize(assimpMesh->GetNumUVChannels());
        mMeshPrims[i].colors.resize(assimpMesh->GetNumColorChannels());
        for (size_t j = 0; j < assimpMesh->mNumFaces; j++)
        {
            const aiFace& face = assimpMesh->mFaces[j];
            if (face.mNumIndices >= 3)
            {
                for (size_t k = 0; k < face.mNumIndices; k++)
                {
                    if (assimpMesh->mNormals)
                    {
                        mMeshPrims[i].normals.push_back(AiVector3dToGfVector3f(assimpMesh->mNormals[face.mIndices[k]]));
                    }

                    for (size_t m = 0; m < mMeshPrims[i].uvs.size(); m++)
                    {
                        mMeshPrims[i].uvs[m].push_back(
                            AiVector3dToGfVector2f(assimpMesh->mTextureCoords[m][face.mIndices[k]]));
                    }

                    for (size_t m = 0; m < mMeshPrims[i].colors.size(); m++)
                    {
                        mMeshPrims[i].colors[m].push_back(AiColor4DToGfVector3f(assimpMesh->mColors[m][face.mIndices[k]]));
                    }
                }
                mMeshPrims[i].faceVertexCounts.push_back(face.mNumIndices);
            }
        }
    }

    auto usdMesh =
        pxr::UsdGeomMesh::Define(usdStage, pxr::SdfPath(omni::importer::urdf::GetNewSdfPathString(usdStage, path)));


    pxr::VtArray<pxr::GfVec3f> allPoints;
    pxr::VtArray<int> allFaceVertexCounts;
    pxr::VtArray<int> allFaceVertexIndices;
    pxr::VtArray<pxr::GfVec3f> allNormals;
    pxr::VtArray<pxr::VtArray<pxr::GfVec2f>> uvs;
    pxr::VtArray<pxr::VtArray<pxr::GfVec3f>> allColors;

    size_t indexOffset = 0;
    size_t vertexOffset = 0;
    std::map<int, pxr::VtArray<int>> materialMap;
    for (size_t m = 0; m < meshTransforms.size(); m++)
    {
        auto transformedMesh = meshTransforms[m];
        auto mesh = mScene->mMeshes[transformedMesh.first];
        auto& meshPrim = mMeshPrims[m];

        for (size_t k = 0; k < meshPrim.uvs.size(); k++)
        {
            uvs.push_back(meshPrim.uvs[k]);
        }

        for (size_t i = 0; i < meshPrim.points.size(); i++)
        {
            allPoints.push_back(meshPrim.points[i]);
        }

        for (size_t i = 0; i < meshPrim.faceVertexCounts.size(); i++)
        {
            allFaceVertexCounts.push_back(meshPrim.faceVertexCounts[i]);
        }

        for (size_t i = 0; i < meshPrim.faceVertexIndices.size(); i++)
        {
            allFaceVertexIndices.push_back(static_cast<int>(meshPrim.faceVertexIndices[i] + indexOffset));
        }
        for (size_t i = vertexOffset; i < vertexOffset + meshPrim.faceVertexCounts.size(); i++)
        {
            materialMap[mesh->mMaterialIndex].push_back(static_cast<int>(i));
        }
        // printf("faceVertexOffset %d %d %d %d\n", indexOffset, points.size(), vertexOffset, faceVertexCounts.size());
        indexOffset = indexOffset + meshPrim.points.size();
        vertexOffset = vertexOffset + meshPrim.faceVertexCounts.size();

        for (size_t i = 0; i < meshPrim.normals.size(); i++)
        {
            allNormals.push_back(meshPrim.normals[i]);
        }
    }

    usdMesh.CreatePointsAttr(pxr::VtValue(allPoints));
    usdMesh.CreateFaceVertexCountsAttr(pxr::VtValue(allFaceVertexCounts));
    usdMesh.CreateFaceVertexIndicesAttr(pxr::VtValue(allFaceVertexIndices));

    pxr::VtArray<pxr::GfVec3f> Extent;
    pxr::UsdGeomPointBased::ComputeExtent(allPoints, &Extent);
    usdMesh.CreateExtentAttr().Set(Extent);

    // Normals
    if (!allNormals.empty())
    {
        usdMesh.CreateNormalsAttr(pxr::VtValue(allNormals));
        usdMesh.SetNormalsInterpolation(pxr::UsdGeomTokens->faceVarying);
    }

    // Texture UV
    for (size_t j = 0; j < uvs.size(); j++)
    {
        pxr::TfToken stName;
        if (j == 0)
        {
            stName = pxr::TfToken("st");
        }
        else
        {
            stName = pxr::TfToken("st_" + std::to_string(j));
        }
        pxr::UsdGeomPrimvarsAPI primvarsAPI(usdMesh);
        pxr::UsdGeomPrimvar Primvar =
            primvarsAPI.CreatePrimvar(stName, pxr::SdfValueTypeNames->TexCoord2fArray, pxr::UsdGeomTokens->faceVarying);
        Primvar.Set(uvs[j]);
    }

    usdMesh.CreateSubdivisionSchemeAttr(pxr::VtValue(pxr::TfToken(subdivisionScheme)));
    if (loadMaterials)
    {
        std::string prefix_path;
        if (instanceable)
        {
            prefix_path = pxr::SdfPath(path).GetParentPath().GetString(); // body category root
        }
        else
        {
            prefix_path = pxr::SdfPath(path).GetParentPath().GetParentPath().GetString(); // Robot root
        }

        // For each material, store the face indices and create GeomSubsets
        usdStage->DefinePrim(pxr::SdfPath(prefix_path + "/Looks"), pxr::TfToken("Scope"));
        for (auto const& mat : materialMap)
        {
            MeshMaterial material(mScene->mMaterials[mat.first]);
            // printf("materials: %s\n", name.c_str());

            pxr::TfToken mat_token(material.get_hash());
            // if (std::find(materialsList.begin(), materialsList.end(),mat_token) == materialsList.end())
            if (materialsList.find(mat_token) == materialsList.end())
            {

                pxr::UsdPrim prim;
                pxr::UsdShadeMaterial matPrim;
                std::string mat_path(prefix_path + "/Looks/" + makeValidUSDIdentifier("material_" + material.name));
                prim = usdStage->GetPrimAtPath(pxr::SdfPath(mat_path));
                int counter = 0;
                while (prim)
                {
                    mat_path = std::string(
                        prefix_path + "/Looks/" +
                        makeValidUSDIdentifier("material_" + material.name + "_" + std::to_string(++counter)));
                    printf("%s \n", mat_path.c_str());
                    prim = usdStage->GetPrimAtPath(pxr::SdfPath(mat_path));
                }
                materialsList[mat_token] = mat_path;
                matPrim = pxr::UsdShadeMaterial::Define(usdStage, pxr::SdfPath(mat_path));
                pxr::UsdShadeShader pbrShader = pxr::UsdShadeShader::Define(usdStage, pxr::SdfPath(mat_path + "/Shader"));
                pbrShader.CreateIdAttr(pxr::VtValue(pxr::UsdImagingTokens->UsdPreviewSurface));

                auto shader_out = pbrShader.CreateOutput(pxr::TfToken("out"), pxr::SdfValueTypeNames->Token);
                matPrim.CreateSurfaceOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                matPrim.CreateVolumeOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                matPrim.CreateDisplacementOutput(pxr::TfToken("mdl")).ConnectToSource(shader_out);
                pbrShader.GetImplementationSourceAttr().Set(pxr::UsdShadeTokens->sourceAsset);
                pbrShader.SetSourceAsset(pxr::SdfAssetPath("OmniPBR.mdl"), pxr::TfToken("mdl"));
                pbrShader.SetSourceAssetSubIdentifier(pxr::TfToken("OmniPBR"), pxr::TfToken("mdl"));
                bool has_emissive_map = false;
                for (int i = 0; i < 5; i++)
                {

                    if (material.texturePaths[i] != "")
                    {
                        if (!usdStage->GetRootLayer()->IsAnonymous())
                        {
                            auto texture_path = copyTexture(usdStage->GetRootLayer()->GetIdentifier(),
                                                            resolve_absolute(base_path, material.texturePaths[i]));
                            int basename_idx = (int)texture_path.rfind('/');
                            std::string filename = texture_path.substr(basename_idx + 1);
                            std::string texture_relative_path = "materials/" + filename;
                            pbrShader.CreateInput(pxr::TfToken(material.props[i]), pxr::SdfValueTypeNames->Asset)
                                .Set(pxr::SdfAssetPath(texture_relative_path));
                            if (material.textures[i] == aiTextureType_EMISSIVE)
                            {
                                pbrShader.CreateInput(pxr::TfToken("emissive_color"), pxr::SdfValueTypeNames->Color3f)
                                    .Set(pxr::GfVec3f(1.0f, 1.0f, 1.0f));
                                pbrShader.CreateInput(pxr::TfToken("enable_emission"), pxr::SdfValueTypeNames->Bool)
                                    .Set(true);
                                pbrShader.CreateInput(pxr::TfToken("emissive_intensity"), pxr::SdfValueTypeNames->Float)
                                    .Set(10000.0f);
                                has_emissive_map = true;
                            }
                        }
                        else
                        {
                            CARB_LOG_WARN(
                                "Material %s has an image texture, but it won't be imported since the asset is being loaded on memory. Please import it into a destination folder to get all textures.",
                                material.name.c_str());
                        }
                    }
                }
                if (material.has_diffuse)
                {
                    pbrShader.CreateInput(pxr::TfToken("diffuse_color_constant"), pxr::SdfValueTypeNames->Color3f)
                        .Set(pxr::GfVec3f(material.diffuse.r, material.diffuse.g, material.diffuse.b));
                }
                if (material.has_metallic)
                {
                    pbrShader.CreateInput(pxr::TfToken("metallic_constant"), pxr::SdfValueTypeNames->Float)
                        .Set(material.metallic);
                }
                if (material.has_specular)
                {
                    pbrShader.CreateInput(pxr::TfToken("specular_level"), pxr::SdfValueTypeNames->Float)
                        .Set(material.specular);
                }
                if (!has_emissive_map && material.has_emissive)
                {
                    pbrShader.CreateInput(pxr::TfToken("emissive_color"), pxr::SdfValueTypeNames->Color3f)
                        .Set(pxr::GfVec3f(material.emissive.r, material.emissive.g, material.emissive.b));
                }


                // auto output = matPrim.CreateSurfaceOutput();
                // output.ConnectToSource(pbrShader, pxr::TfToken("surface"));
            }
            pxr::UsdShadeMaterial matPrim =
                pxr::UsdShadeMaterial(usdStage->GetPrimAtPath(pxr::SdfPath(materialsList[mat_token])));
            if (materialMap.size() > 1)
            {

                auto geomSubset = pxr::UsdGeomSubset::Define(
                    usdStage, pxr::SdfPath(usdMesh.GetPath().GetString() + "/material_" + std::to_string(mat.first)));
                geomSubset.CreateElementTypeAttr(pxr::VtValue(pxr::TfToken("face")));
                geomSubset.CreateFamilyNameAttr(pxr::VtValue(pxr::TfToken("materialBind")));
                geomSubset.CreateIndicesAttr(pxr::VtValue(mat.second));
                if (matPrim)
                {
                    pxr::UsdShadeMaterialBindingAPI mbi(geomSubset);
                    mbi.Bind(matPrim);
                }
            }
            else
            {
                if (matPrim)
                {
                    pxr::UsdShadeMaterialBindingAPI mbi(usdMesh);
                    mbi.Bind(matPrim);
                }
            }
        }
    }

    return usdMesh.GetPath();
}
}
}
}
