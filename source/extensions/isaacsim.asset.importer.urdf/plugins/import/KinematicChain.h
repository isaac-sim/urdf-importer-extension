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

#include "../UrdfTypes.h"

#include <memory>
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
// Represents the kinematic chain as a tree
class KinematicChain
{
public:
    // A tree representing a link with its parent joint and child links
    struct Node
    {
        std::string linkName_;
        std::string parentJointName_;
        std::vector<std::unique_ptr<Node>> childNodes_;

        Node(std::string linkName, std::string parentJointName) : linkName_(linkName), parentJointName_(parentJointName)
        {
        }
    };

    std::unique_ptr<Node> baseNode;

    KinematicChain() = default;

    ~KinematicChain();

    // Computes the kinematic chain for a UrdfRobot description
    bool computeKinematicChain(const UrdfRobot& urdfRobot);

private:
    // Recursively finds a node's children
    void computeChildNodes(std::unique_ptr<Node>& parentNode, const UrdfRobot& urdfRobot);
};

}
}
}
}
