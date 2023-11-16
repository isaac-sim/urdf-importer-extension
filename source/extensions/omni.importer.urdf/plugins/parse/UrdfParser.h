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

#include "../UrdfTypes.h"

#include <tinyxml2.h>
namespace omni
{
namespace importer
{
namespace urdf
{

// Parsers

bool parseJointType(const std::string& str, UrdfJointType& type);

bool parseOrigin(const tinyxml2::XMLElement& element, Transform& origin);

bool parseAxis(const tinyxml2::XMLElement& element, UrdfAxis& axis);

bool parseLimit(const tinyxml2::XMLElement& element, UrdfLimit& limit);

bool parseDynamics(const tinyxml2::XMLElement& element, UrdfDynamics& dynamics);

bool parseMass(const tinyxml2::XMLElement& element, float& mass);

bool parseInertia(const tinyxml2::XMLElement& element, UrdfInertia& inertia);

bool parseInertial(const tinyxml2::XMLElement& element, UrdfInertial& inertial);

bool parseGeometry(const tinyxml2::XMLElement& element, UrdfGeometry& geometry);

bool parseMaterial(const tinyxml2::XMLElement& element, UrdfMaterial& material);

bool parseMaterials(const tinyxml2::XMLElement& root, std::map<std::string, UrdfMaterial>& urdfMaterials);

bool parseLinks(const tinyxml2::XMLElement& root, std::map<std::string, UrdfLink>& urdfLinks);

bool parseJoints(const tinyxml2::XMLElement& root, std::map<std::string, UrdfJoint>& urdfJoints);

bool parseUrdf(const std::string& urdfPackagePath, const std::string& urdfFileRelativeToPackage, UrdfRobot& urdfRobot);

bool findRootLink(const std::map<std::string, UrdfLink>& urdfLinks, const std::map<std::string, UrdfJoint>& urdfJoints, std::string& rootLinkName);

} // namespace urdf
}
}
