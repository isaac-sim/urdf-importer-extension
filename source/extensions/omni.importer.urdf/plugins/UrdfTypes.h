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

#include "math/core/maths.h"

#include <float.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace omni
{
namespace importer
{
namespace urdf
{

// The default values and data structures are mostly the same as defined in the official URDF documentation
// http://wiki.ros.org/urdf/XML

struct UrdfInertia
{
    float ixx = 0.0f;
    float ixy = 0.0f;
    float ixz = 0.0f;
    float iyy = 0.0f;
    float iyz = 0.0f;
    float izz = 0.0f;
};

struct UrdfInertial
{
    Transform origin; // This is the pose of the inertial reference frame, relative to the link reference frame. The
                      // origin of the inertial reference frame needs to be at the center of gravity
    float mass = 0.0f;
    UrdfInertia inertia;
    bool hasOrigin = false;
    bool hasMass = false; // Whether the inertial field defined a mass
    bool hasInertia = false; // Whether the inertial field defined an inertia
};

struct UrdfAxis
{
    float x = 1.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// By Default a UrdfColor struct will have an invalid color unless it was found in the xml
struct UrdfColor
{
    float r = -1.0f;
    float g = -1.0f;
    float b = -1.0f;
    float a = 1.0f;
};

enum class UrdfJointType
{
    REVOLUTE = 0, // A hinge joint that rotates along the axis and has a limited range specified by the upper and lower
                  // limits
    CONTINUOUS = 1, // A continuous hinge joint that rotates around the axis and has no upper and lower limits
    PRISMATIC = 2, // A sliding joint that slides along the axis, and has a limited range specified by the upper and
                   // lower limits
    FIXED = 3, // this is not really a joint because it cannot move. All degrees of freedom are locked. This type of
               // joint does not require the axis, calibration, dynamics, limits or safety_controller
    FLOATING = 4, // This joint allows motion for all 6 degrees of freedom
    PLANAR = 5 // This joint allows motion in a plane perpendicular to the axis
};

enum class UrdfJointTargetType
{
    NONE = 0,
    POSITION = 1,
    VELOCITY = 2
};


enum class UrdfNormalSubdivisionScheme
{
    CATMULLCLARK = 0,
    LOOP = 1,
    BILINEAR = 2,
    NONE = 3
};

enum class UrdfJointDriveType
{
    ACCELERATION = 0,
    FORCE = 2
};

enum class UrdfSensorType
{
    CAMERA = 0,
    RAY = 1,
    IMU = 2,
    MAGNETOMETER = 3,
    GPS = 4,
    FORCE = 5,
    CONTACT = 6,
    SONAR = 7,
    RFIDTAG = 8,
    RFID = 9,
    UNSUPPORTED = -1,

};

struct UrdfDynamics
{
    float damping = 0.0f;
    float friction = 0.0f;
    float stiffness = 0.0f;
};

struct UrdfJointDrive
{
    float target = 0.0;
    float strength = 0.0f;
    float damping = 0.0f;
    UrdfJointTargetType targetType = UrdfJointTargetType::POSITION;
    UrdfJointDriveType driveType = UrdfJointDriveType::FORCE;
};

struct UrdfJointMimic
{
    std::string joint = "";
    float multiplier;
    float offset;
};

struct UrdfLimit
{
    float lower = -FLT_MAX; // An attribute specifying the lower joint limit (radians for revolute joints, meters for
                            // prismatic joints)
    float upper = FLT_MAX; // An attribute specifying the upper joint limit (radians for revolute joints, meters for
                           // prismatic joints)
    float effort = FLT_MAX; // An attribute for enforcing the maximum joint effort
    float velocity = FLT_MAX; // An attribute for enforcing the maximum joint velocity
};

enum class UrdfGeometryType
{
    BOX = 0,
    CYLINDER = 1,
    CAPSULE = 2,
    SPHERE = 3,
    MESH = 4
};

struct UrdfGeometry
{
    UrdfGeometryType type;
    // Box
    float size_x = 0.0f;
    float size_y = 0.0f;
    float size_z = 0.0f;
    // Cylinder and Sphere
    float radius = 0.0f;
    float length = 0.0f;
    // Mesh
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    float scale_z = 1.0f;
    std::string meshFilePath;
};

struct UrdfMaterial
{
    std::string name;
    UrdfColor color;
    std::string textureFilePath;
};

struct UrdfVisual
{
    std::string name;
    Transform origin; // The reference frame of the visual element with respect to the reference frame of the link
    UrdfGeometry geometry;
    UrdfMaterial material;
};

struct UrdfCollision
{
    std::string name;
    Transform origin; // The reference frame of the collision element, relative to the reference frame of the link
    UrdfGeometry geometry;
};

struct UrdfNoise
{
    float mean;
    float stddev;
    float biasMean;
    float biasStddev;
    float precision;
};

struct UrdfSensor
{
    std::string name;
    Transform origin;
    UrdfSensorType type;
    std::string id;
    float updateRate;
};


struct UrdfCamera : public UrdfSensor
{
    float width;
    float height;
    std::string format;
    float hfov;
    float clipNear;
    float clipFar;
};

struct UrdfRayDim
{
    int samples;
    float resolution;
    float minAngle;
    float maxAngle;
};

struct UrdfRay : public UrdfSensor
{
    bool hasHorizontal = false;
    bool hasVertical = false;
    UrdfRayDim horizontal;
    UrdfRayDim vertical;
    std::string isaacSimConfig;
    // float min;
    // float max;
    // float resolution;
};

struct UrdfImu : public UrdfSensor
{
    UrdfNoise gyroNoise;
    UrdfNoise accelerationNoise;
};

struct UrdfMagnetometer : public UrdfSensor
{
    UrdfNoise noise;
};

struct UrdfGps : public UrdfSensor
{
    UrdfNoise positionNoise;
    UrdfNoise velocityNoise;
};

struct UrdfForce : public UrdfSensor
{
    std::string frame; // the child element to measure force
    int measureDirection; // 0 is parent_to_child, 1 is child_to_parent
};

struct UrdfContact : public UrdfSensor
{
    std::vector<UrdfCollision> collision;
};

struct UrdfSonar : public UrdfSensor
{
    float min;
    float max;
    float radius;
};

struct UrdfRfidTag : public UrdfSensor
{
};

struct UrdfRfid : public UrdfSensor
{
};
struct UrdfLink
{
    std::string name;
    UrdfInertial inertial;
    std::vector<UrdfVisual> visuals;
    std::vector<UrdfCollision> collisions;
    std::map<std::string, Transform> mergedChildren;
    std::vector<UrdfCamera> cameras;
    std::vector<UrdfRay> lidars;

    // std::vector<UrdfMagnetometer> magnetometers;
    // std::vector<UrdfImu> Imus;
    // std::vector<UrdfGps> Gps;
    // std::vector<UrdfForce> forceSensors;
    // std::vector<UrdfContact> contactSensors;
    // std::vector<UrdfSonar> sonars;
    // std::vector<UrdfRfid> rfids;
    // std::vector<UrdfRfidTag> rfdidTags;
};

struct UrdfJoint
{
    std::string name;
    UrdfJointType type;
    Transform origin; // This is the transform from the parent link to the child link. The joint is located at the
                      // origin of the child link
    std::string parentLinkName;
    std::string childLinkName;
    UrdfAxis axis;
    UrdfDynamics dynamics;
    UrdfLimit limit;
    UrdfJointDrive drive;
    UrdfJointMimic mimic;
    std::map<std::string, float> mimicChildren;

    bool dontCollapse = false; // This is a custom attribute that is used to prevent the child link from being
                               // collapsed into the parent link when a fixed joint is used. It is used when user
                               // enables "merging of fixed joints" but does not want to merge this particular joint.
                               // For example: for sensor or end-effector frames.
                               // Note: The tag is not part of the URDF specification. Rather it is a custom tag
                               //   that was first introduced in Isaac Gym for the purpose of merging fixed joints.
};

struct UrdfRobot
{
    std::string name;
    std::string rootLink;
    std::map<std::string, UrdfLink> links;
    std::map<std::string, UrdfJoint> joints;
    std::map<std::string, UrdfMaterial> materials;
};


} // namespace urdf
}
}
