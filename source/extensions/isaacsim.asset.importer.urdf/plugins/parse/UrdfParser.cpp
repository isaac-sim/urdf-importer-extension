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

#include "UrdfParser.h"

#include "../core/PathUtils.h"

#include <carb/logging/Log.h>

#include <sstream>
#include <stdexcept>

namespace isaacsim
{
namespace asset
{
namespace importer
{
namespace urdf
{
float kDesiredNaturalFrequency = 4.0f;

// Stream operators for nice printing
std::ostream& operator<<(std::ostream& out, const Transform& origin)
{
    out << "Origin: ";
    out << "px=" << origin.p.x << " py=" << origin.p.y << " pz=" << origin.p.z;
    out << " qx=" << origin.q.x << " qy=" << origin.q.y << " qz=" << origin.q.z << " qw=" << origin.q.w;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfInertia& inertia)
{
    out << "Inertia: ";
    out << "ixx=" << inertia.ixx << " ixy=" << inertia.ixy << " ixz=" << inertia.ixz;
    out << " iyy=" << inertia.iyy << " iyz=" << inertia.iyz << " izz=" << inertia.izz;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfInertial& inertial)
{
    out << "Inertial: " << std::endl;
    out << " \t \t" << inertial.origin << std::endl;
    if (inertial.hasMass)
    {
        out << " \t \tMass: " << inertial.mass << std::endl;
    }
    else
    {
        out << " \t \tMass: No mass was specified for the link" << std::endl;
    }
    if (inertial.hasInertia)
    {
        out << " \t \t" << inertial.inertia;
    }
    else
    {
        out << " \t \tInertia: No inertia was specified for the link" << std::endl;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfAxis& axis)
{
    out << "Axis: ";
    out << "x=" << axis.x << " y=" << axis.y << " z=" << axis.z;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfColor& color)
{
    out << "Color: ";
    out << "r=" << color.r << " g=" << color.g << " b=" << color.b << " a=" << color.a;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfJointType& type)
{
    out << "Type: ";
    std::string jointType = "unknown";
    switch (type)
    {
    case UrdfJointType::REVOLUTE:
        jointType = "revolute";
        break;
    case UrdfJointType::CONTINUOUS:
        jointType = "continuous";
        break;
    case UrdfJointType::PRISMATIC:
        jointType = "prismatic";
        break;
    case UrdfJointType::FIXED:
        jointType = "fixed";
        break;
    case UrdfJointType::FLOATING:
        jointType = "floating";
        break;
    case UrdfJointType::PLANAR:
        jointType = "planar";
        break;
    case UrdfJointType::SPHERICAL:
        jointType = "spherical";
        break;
    }
    out << jointType;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfDynamics& dynamics)
{
    out << "Dynamics: ";
    out << "damping=" << dynamics.damping << " friction=" << dynamics.friction;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfLimit& limit)
{
    out << "Limit: ";
    out << "lower=" << limit.lower << " upper=" << limit.upper << " effort=" << limit.effort
        << " velocity=" << limit.velocity;
    out << std::endl;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfGeometry& geometry)
{
    out << "Geometry: ";
    switch (geometry.type)
    {
    case UrdfGeometryType::BOX:
        out << "type=box size=" << geometry.size_x << " " << geometry.size_y << " " << geometry.size_z;
        break;
    case UrdfGeometryType::CYLINDER:
        out << "type=cylinder radius=" << geometry.radius << " length=" << geometry.length;
        break;
    case UrdfGeometryType::CAPSULE:
        out << "type=capsule radius=" << geometry.radius << " length=" << geometry.length;
        break;
    case UrdfGeometryType::SPHERE:
        out << "type=sphere, radius=" << geometry.radius;
        break;
    case UrdfGeometryType::MESH:
        out << "type=mesh filemame=" << geometry.meshFilePath;
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfMaterial& material)
{
    out << "Material: ";
    out << " Name=" << material.name << " " << material.color;
    if (!material.textureFilePath.empty())
        out << " textureFilePath=" << material.textureFilePath;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfVisual& visual)
{
    out << "Visual:" << std::endl;
    if (!visual.name.empty())
        out << " \t \tName=" << visual.name << std::endl;
    out << " \t \t" << visual.origin << std::endl;
    out << " \t \t" << visual.geometry << std::endl;
    out << " \t \t" << visual.material;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfCollision& collision)
{
    out << "Collision:" << std::endl;
    if (!collision.name.empty())
        out << " \t \tName=" << collision.name << std::endl;
    out << " \t \t" << collision.origin << std::endl;
    out << " \t \t" << collision.geometry;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfLink& link)
{
    out << "Link: ";
    out << " \tName=" << link.name << std::endl;
    for (auto& visual : link.visuals)
    {
        out << " \t" << visual << std::endl;
    }
    for (auto& collision : link.collisions)
    {
        out << " \t" << collision << std::endl;
    }
    out << " \t" << link.inertial << std::endl;
    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfJoint& joint)
{
    out << "Joint: ";
    out << " Name=" << joint.name << std::endl;
    out << " \t" << joint.type << std::endl;
    out << " \t" << joint.origin << std::endl;
    out << " \tParentLinkName=" << joint.parentLinkName << std::endl;
    out << " \tChildLinkName=" << joint.childLinkName << std::endl;
    out << " \t" << joint.axis << std::endl;
    out << " \t" << joint.dynamics << std::endl;
    out << " \t" << joint.limit << std::endl;
    out << " \t" << joint.dontCollapse << std::endl;

    return out;
}

std::ostream& operator<<(std::ostream& out, const UrdfRobot& robot)
{
    out << "Robot: ";
    out << robot.name << std::endl;
    for (auto& link : robot.links)
    {
        out << link.second << std::endl;
    }
    for (auto& joint : robot.joints)
    {
        out << joint.second << std::endl;
    }
    for (auto& material : robot.materials)
    {
        out << material.second << std::endl;
    }
    return out;
}

using namespace tinyxml2;


bool parseXyz(const char* str, float& x, float& y, float& z)
{
    std::istringstream iss(str);
    if (iss >> x >> y >> z)
    {
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Could not parse xyz string '%s' ", str);
        return false;
    }
}

bool parseColor(const char* str, float& r, float& g, float& b, float& a)
{
    std::istringstream iss(str);
    if (iss >> r >> g >> b >> a)
    {
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Could not parse color string '%s' ", str);
        return false;
    }
}

bool parseFloat(const char* str, float& value)
{
    std::istringstream iss(str);
    if (iss >> value)
    {
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Could not parse float string '%s' ", str);
        return false;
    }
}

bool parseSize(const char* str, size_t& value)
{
    std::istringstream iss(str);
    if (iss >> value)
    {
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Could not parse int string '%s' ", str);
        return false;
    }
}

bool parseInt(const char* str, int& value)
{
    std::istringstream iss(str);
    if (iss >> value)
    {
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Could not parse int string '%s' ", str);
        return false;
    }
}

bool parseFloatPair(const std::string& str, float& a, float& b)
{
    std::istringstream iss(str);
    return (iss >> a >> b) && !iss.fail();
}
// Create a map to store the joint types
const std::unordered_map<std::string, UrdfJointType> jointTypes = {
    { "revolute", UrdfJointType::REVOLUTE },   { "continuous", UrdfJointType::CONTINUOUS },
    { "prismatic", UrdfJointType::PRISMATIC }, { "fixed", UrdfJointType::FIXED },
    { "floating", UrdfJointType::FLOATING },   { "planar", UrdfJointType::PLANAR },
    { "spherical", UrdfJointType::SPHERICAL }
};

bool parseJointType(const char* str, UrdfJointType& type)
{
    // Convert the input string to a std::string
    std::string strCopy(str);

    // Check if the string is in the map
    auto it = jointTypes.find(strCopy);
    if (it != jointTypes.end())
    {
        type = it->second;
        return true;
    }
    else
    {
        CARB_LOG_ERROR("*** Unknown joint type '%s' ", str);
        return false;
    }
}

bool parseJointMimic(const XMLElement& element, UrdfJointMimic& mimic)
{
    auto jointElement = element.Attribute("joint");
    if (jointElement)
    {
        mimic.joint = std::string(jointElement);
    }

    auto multiplierElement = element.Attribute("multiplier");
    if (!multiplierElement || !parseFloat(multiplierElement, mimic.multiplier))
    {
        mimic.multiplier = 1.0f;
    }

    auto offsetElement = element.Attribute("offset");
    if (!offsetElement || !parseFloat(offsetElement, mimic.offset))
    {
        mimic.offset = 0;
    }

    return true;
}

bool parseOrigin(const XMLElement& element, Transform& origin)
{
    auto originElement = element.FirstChildElement("origin");
    if (originElement)
    {
        auto attribute = originElement->Attribute("xyz");
        if (attribute)
        {
            if (!parseXyz(attribute, origin.p.x, origin.p.y, origin.p.z))
            {
                // optional, use zero vector
                origin.p = Vec3(0.0);
            }
        }
        attribute = originElement->Attribute("rpy");
        if (attribute)
        {
            // parse roll pitch yaw
            float roll = 0.0f;
            float pitch = 0.0f;
            float yaw = 0.0f;
            if (!parseXyz(attribute, roll, pitch, yaw))
            {
                roll = pitch = yaw = 0.0;
            }
            // convert to transform quaternion:
            origin.q = rpy2quat(roll, pitch, yaw);
        }
        return true;
    }
    return false;
}

bool parseAxis(const XMLElement& element, UrdfAxis& axis)
{
    auto axisElement = element.FirstChildElement("axis");
    if (axisElement)
    {
        auto attribute = axisElement->Attribute("xyz");
        if (attribute)
        {
            if (!parseXyz(attribute, axis.x, axis.y, axis.z))
            {
                CARB_LOG_ERROR("*** xyz not specified for axis");
                return false;
            }
        }
    }
    return true;
}

bool parseLimit(const XMLElement& element, UrdfLimit& limit)
{
    auto limitElement = element.FirstChildElement("limit");
    if (limitElement)
    {
        auto attribute = limitElement->Attribute("lower");
        if (attribute)
        {
            if (!parseFloat(attribute, limit.lower))
            {
                // optional, use zero if not specified
                limit.lower = 0.0;
            }
        }
        attribute = limitElement->Attribute("upper");
        if (attribute)
        {
            if (!parseFloat(attribute, limit.upper))
            {
                // optional, use zero if not specified
                limit.upper = 0.0;
            }
        }
        attribute = limitElement->Attribute("effort");
        if (attribute)
        {
            if (!parseFloat(attribute, limit.effort))
            {
                CARB_LOG_ERROR("*** effort not specified for limit");
                return false;
            }
        }
        attribute = limitElement->Attribute("velocity");
        if (attribute)
        {
            if (!parseFloat(attribute, limit.velocity))
            {
                CARB_LOG_ERROR("*** velocity not specified for limit");
                return false;
            }
        }
    }
    return true;
}

bool parseDynamics(const XMLElement& element, UrdfDynamics& dynamics)
{
    auto dynamicsElement = element.FirstChildElement("dynamics");
    if (dynamicsElement)
    {
        auto attribute = dynamicsElement->Attribute("damping");
        if (attribute)
        {
            if (!parseFloat(attribute, dynamics.damping))
            {
                // optional
                dynamics.damping = 0.0f;
            }
        }
        attribute = dynamicsElement->Attribute("friction");
        if (attribute)
        {
            if (!parseFloat(attribute, dynamics.friction))
            {
                // optional
                dynamics.friction = 0.0f;
            }
        }
        attribute = dynamicsElement->Attribute("spring_stiffness");
        if (attribute)
        {
            if (!parseFloat(attribute, dynamics.stiffness))
            {
                // optional
                dynamics.stiffness = 0.0f;
            }
        }
    }
    return true;
}

bool parseMass(const XMLElement& element, float& mass)
{
    auto massElement = element.FirstChildElement("mass");
    if (massElement)
    {
        auto attribute = massElement->Attribute("value");
        if (attribute)
        {
            if (!parseFloat(attribute, mass))
            {
                CARB_LOG_ERROR("*** couldn't parse mass ");
                return false;
            }
            return true;
        }
        else
        {
            CARB_LOG_ERROR("*** mass missing from inertia ");
            return false;
        }
    }
    return false;
}

bool parseInertia(const XMLElement& element, UrdfInertia& inertia)
{
    auto inertiaElement = element.FirstChildElement("inertia");
    if (inertiaElement)
    {
        auto attribute = inertiaElement->Attribute("ixx");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.ixx))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** ixx missing from inertia ");
            return false;
        }

        attribute = inertiaElement->Attribute("ixz");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.ixz))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** ixz missing from inertia ");
            return false;
        }

        attribute = inertiaElement->Attribute("ixy");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.ixy))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** ixy missing from inertia ");
            return false;
        }

        attribute = inertiaElement->Attribute("iyy");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.iyy))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** iyy missing from inertia ");
            return false;
        }

        attribute = inertiaElement->Attribute("iyz");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.iyz))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** iyz missing from inertia ");
            return false;
        }

        attribute = inertiaElement->Attribute("izz");
        if (attribute)
        {
            if (!parseFloat(attribute, inertia.izz))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** izz missing from inertia ");
            return false;
        }

        // if we made it here, all elements were parsed successfully
        return true;
    }
    return false;
}

bool parseInertial(const XMLElement& element, UrdfInertial& inertial)
{
    auto inertialElement = element.FirstChildElement("inertial");
    if (inertialElement)
    {
        inertial.hasOrigin = parseOrigin(*inertialElement, inertial.origin);
        inertial.hasMass = parseMass(*inertialElement, inertial.mass);
        inertial.hasInertia = parseInertia(*inertialElement, inertial.inertia);
    }
    return true;
}

bool parseGeometry(const XMLElement& element, UrdfGeometry& geometry)
{
    auto geometryElement = element.FirstChildElement("geometry");
    if (geometryElement)
    {
        auto geometryElementChild = geometryElement->FirstChildElement();

        if (strcmp(geometryElementChild->Value(), "mesh") == 0)
        {
            geometry.type = UrdfGeometryType::MESH;

            auto filename = geometryElementChild->Attribute("filename");
            if (filename)
            {
                geometry.meshFilePath = filename;
            }
            else
            {
                CARB_LOG_ERROR("*** mesh geometry requires a file path ");
                return false;
            }

            auto scale = geometryElementChild->Attribute("scale");
            if (scale)
            {
                if (!parseXyz(scale, geometry.scale_x, geometry.scale_y, geometry.scale_z))
                {
                    CARB_LOG_ERROR("*** scale is missing xyz ");
                    return false;
                }
            }
        }
        else if (strcmp(geometryElementChild->Value(), "box") == 0)
        {
            geometry.type = UrdfGeometryType::BOX;
            auto attribute = geometryElementChild->Attribute("size");
            if (attribute)
            {
                if (!parseXyz(attribute, geometry.size_x, geometry.size_y, geometry.size_z))
                {
                    CARB_LOG_ERROR("*** couldn't parse xyz size ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** box geometry requires a size ");
                return false;
            }
        }
        else if (strcmp(geometryElementChild->Value(), "cylinder") == 0)
        {
            geometry.type = UrdfGeometryType::CYLINDER;
            auto attribute = geometryElementChild->Attribute("radius");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.radius))
                {
                    CARB_LOG_ERROR("*** couldn't parse radius ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** cylinder geometry requires a radius ");
                return false;
            }

            attribute = geometryElementChild->Attribute("length");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.length))
                {
                    CARB_LOG_ERROR("*** couldn't parse length ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** cylinder geometry requires a length ");
                return false;
            }
        }
        else if (strcmp(geometryElementChild->Value(), "capsule") == 0)
        {
            geometry.type = UrdfGeometryType::CAPSULE;
            auto attribute = geometryElementChild->Attribute("radius");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.radius))
                {
                    CARB_LOG_ERROR("*** couldn't parse radius ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** capsule geometry requires a radius ");
                return false;
            }

            attribute = geometryElementChild->Attribute("length");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.length))
                {
                    CARB_LOG_ERROR("*** couldn't parse length ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** capsule geometry requires a length ");
                return false;
            }
        }
        else if (strcmp(geometryElementChild->Value(), "sphere") == 0)
        {
            geometry.type = UrdfGeometryType::SPHERE;
            auto attribute = geometryElementChild->Attribute("radius");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.radius))
                {
                    CARB_LOG_ERROR("*** couldn't parse radius ");
                    return false;
                }
            }
            else
            {
                CARB_LOG_ERROR("*** sphere geometry requires a radius ");
                return false;
            }
        }
    }

    return true;
}

bool parseChildAttributeFloat(const tinyxml2::XMLElement& element, const char* child, const char* attribute, float& output)
{
    auto childElement = element.FirstChildElement(child);

    if (childElement)
    {
        const char* s = childElement->Attribute(attribute);
        if (s)
        {
            return parseFloat(s, output);
        }
    }

    return false;
}

bool parseChildAttributeString(const tinyxml2::XMLElement& element,
                               const char* child,
                               const char* attribute,
                               std::string& output)
{
    auto childElement = element.FirstChildElement(child);

    if (childElement)
    {
        const char* s = childElement->Attribute(attribute);
        if (s)
        {
            output = s;
            return true;
        }
    }

    return false;
}


// Convert const char* to UrdfSensorType
UrdfSensorType stringToSensorType(const char* str)
{
    // Map strings to enum values
    static const std::unordered_map<std::string, UrdfSensorType> stringToSensorType = {
        { "camera", UrdfSensorType::CAMERA }, { "ray", UrdfSensorType::RAY },
        // {"magnetometer",  UrdfSensorType::MAGNETOMETER},
        // {"imu",  UrdfSensorType::IMU},
        // {"gps",  UrdfSensorType::GPS},
        // {"force_torque",  UrdfSensorType::FORCE},
        // {"contact",  UrdfSensorType::CONTACT},
        // {"sonar",  UrdfSensorType::SONAR},
        // {"rfidtag",  UrdfSensorType::RFIDTAG},
        // {"rfid",  UrdfSensorType::RFID},
    };
    std::string key(str);
    auto it = stringToSensorType.find(key);
    if (it != stringToSensorType.end())
    {
        return it->second;
    }
    else
    {
        return UrdfSensorType::UNSUPPORTED;
    }
}

bool parseSensorType(const tinyxml2::XMLElement& element, UrdfSensorType& output)
{
    const char* s = element.Attribute("type");
    if (s)
    {
        output = stringToSensorType(s);
        if (output == UrdfSensorType::UNSUPPORTED)
        {
            return false;
        }
        return true;
    }
    return false;
}


// bool parseFem(const tinyxml2::XMLElement& element, UrdfFem& fem)
// {
//     parseOrigin(element, fem.origin);

//     if (!parseChildAttributeFloat(element, "density", "value", fem.density))
//         fem.density = 1000.0f;

//     if (!parseChildAttributeFloat(element, "youngs", "value", fem.youngs))
//         fem.youngs = 1.e+4f;

//     if (!parseChildAttributeFloat(element, "poissons", "value", fem.poissons))
//         fem.poissons = 0.3f;

//     if (!parseChildAttributeFloat(element, "damping", "value", fem.damping))
//         fem.damping = 0.0f;

//     if (!parseChildAttributeFloat(element, "attachDistance", "value", fem.attachDistance))
//         fem.attachDistance = 0.0f;

//     if (!parseChildAttributeString(element, "tetmesh", "filename", fem.meshFilePath))
//         fem.meshFilePath = "";

//     if (!parseChildAttributeFloat(element, "scale", "value", fem.scale))
//         fem.scale = 1.0f;

//     return true;
// }

bool parseMaterial(const XMLElement& element, UrdfMaterial& material)
{
    auto materialElement = element.FirstChildElement("material");
    if (materialElement)
    {
        auto name = materialElement->Attribute("name");
        if (name && strlen(name) > 0)
        {
            material.name = makeValidUSDIdentifier(name);
        }
        else
        {
            if (materialElement->FirstChildElement("color"))
            {
                if (!parseColor(materialElement->FirstChildElement("color")->Attribute("rgba"), material.color.r,
                                material.color.g, material.color.b, material.color.a))
                {
                    // optional
                    material.color = UrdfColor();
                }
            }
            if (materialElement->FirstChildElement("texture"))
            {
                auto matString = materialElement->FirstChildElement("texture")->Attribute("filename");
                if (matString == nullptr)
                {
                    CARB_LOG_ERROR("*** filename required for material with texture ");
                    return false;
                }
                material.textureFilePath = matString;
            }
        }
    }

    return true;
}

bool parseMaterials(const XMLElement& root, std::map<std::string, UrdfMaterial>& urdfMaterials)
{
    auto materialElement = root.FirstChildElement("material");
    if (materialElement)
    {
        do
        {
            UrdfMaterial material;
            auto name = materialElement->Attribute("name");
            if (name)
            {
                material.name = makeValidUSDIdentifier(name);
            }
            else
            {
                CARB_LOG_ERROR("*** Found unnamed material ");
                return false;
            }

            auto elem = materialElement->FirstChildElement("color");
            if (elem)
            {
                if (!parseColor(elem->Attribute("rgba"), material.color.r, material.color.g, material.color.b,
                                material.color.a))
                {
                    return false;
                }
            }
            elem = materialElement->FirstChildElement("texture");
            if (elem)
            {
                auto matString = elem->Attribute("filename");
                if (matString == nullptr)
                {
                    CARB_LOG_ERROR("*** filename required for material with texture ");
                    return false;
                }
            }

            urdfMaterials.emplace(material.name, material);

        } while ((materialElement = materialElement->NextSiblingElement("material")));
    }
    return true;
}

bool parseLinks(const XMLElement& root, std::map<std::string, UrdfLink>& urdfLinks)
{
    auto linkElement = root.FirstChildElement("link");
    if (linkElement)
    {
        do
        {
            UrdfLink link;

            // name
            auto name = linkElement->Attribute("name");
            if (name)
            {
                link.name = makeValidUSDIdentifier(name);
            }
            else
            {
                CARB_LOG_ERROR("*** Found unnamed link ");
                return false;
            }

            // visuals
            auto visualElement = linkElement->FirstChildElement("visual");
            if (visualElement)
            {
                do
                {
                    UrdfVisual visual;
                    auto name = visualElement->Attribute("name");
                    if (name)
                    {
                        visual.name = makeValidUSDIdentifier(name);
                    }

                    if (!parseOrigin(*visualElement, visual.origin))
                    {
                        // optional default to identity transform
                        visual.origin = Transform();
                    }

                    if (!parseGeometry(*visualElement, visual.geometry))
                    {
                        CARB_LOG_ERROR("*** Found visual without geometry ");
                        return false;
                    }

                    if (!parseMaterial(*visualElement, visual.material))
                    {
                        // optional, use default if not specified
                        visual.material = UrdfMaterial();
                    }

                    link.visuals.push_back(visual);

                } while ((visualElement = visualElement->NextSiblingElement("visual")));
            }

            // collisions
            auto collisionElement = linkElement->FirstChildElement("collision");
            if (collisionElement)
            {
                do
                {
                    UrdfCollision collision;
                    auto name = collisionElement->Attribute("name");
                    if (name)
                    {
                        collision.name = makeValidUSDIdentifier(name);
                    }

                    if (!parseOrigin(*collisionElement, collision.origin))
                    {
                        // optional default to identity transform
                        collision.origin = Transform();
                    }

                    if (!parseGeometry(*collisionElement, collision.geometry))
                    {
                        CARB_LOG_ERROR("*** Found collision without geometry ");
                        return false;
                    }

                    link.collisions.push_back(collision);

                } while ((collisionElement = collisionElement->NextSiblingElement("collision")));
            }

            // inertia
            if (!parseInertial(*linkElement, link.inertial))
            {
                // optional, use default if not specified
                link.inertial = UrdfInertial();
            }

            // auto femElement = linkElement->FirstChildElement("fem");
            // if (femElement)
            // {
            //     do
            //     {
            //         UrdfFem fem;
            //         if (!parseFem(*femElement, fem))
            //         {
            //             return false;
            //         }

            //         link.softs.push_back(fem);

            //     } while ((femElement = femElement->NextSiblingElement("fem")));
            // }

            urdfLinks.emplace(link.name, link);

        } while ((linkElement = linkElement->NextSiblingElement("link")));
    }
    return true;
}


float GetInertiaMagnitudeOrMass(const UrdfLink& link)
{
    if (link.inertial.hasInertia)
    {
        Matrix33 inertiaMatrix;
        inertiaMatrix.cols[0] = Vec3(link.inertial.inertia.ixx, link.inertial.inertia.ixy, link.inertial.inertia.ixz);
        inertiaMatrix.cols[1] = Vec3(link.inertial.inertia.ixy, link.inertial.inertia.iyy, link.inertial.inertia.iyz);
        inertiaMatrix.cols[2] = Vec3(link.inertial.inertia.ixz, link.inertial.inertia.iyz, link.inertial.inertia.izz);

        return L2_magnitude(inertiaMatrix);
    }
    else if (link.inertial.hasMass)
    {
        return link.inertial.mass;
    }
    else
    {
        // TODO: Try a bit more, use density and volume to calculate mass, Use volume to calculate inertia...
        return 0.0f;
    }
}

float computeEquivalentInertia(const float m1, const float m2)
{
    if (m1 + m2 > 0)
    {
        return m1 * m2 / (m1 + m2);
    }
    return 0;
}


Matrix33 InertiaMatrix(const UrdfInertia& inertia)
{
    Matrix33 inertiaMatrix;
    inertiaMatrix.cols[0].x = inertia.ixx;
    inertiaMatrix.cols[0].y = inertia.ixy;
    inertiaMatrix.cols[0].z = inertia.ixz;
    inertiaMatrix.cols[1].x = inertia.ixy;
    inertiaMatrix.cols[1].y = inertia.iyy;
    inertiaMatrix.cols[1].z = inertia.iyz;
    inertiaMatrix.cols[2].x = inertia.ixz;
    inertiaMatrix.cols[2].y = inertia.iyz;
    inertiaMatrix.cols[2].z = inertia.izz;
    return inertiaMatrix;
}


Matrix33 compute_parallel_axis_inertia(const UrdfLink& link, const Vec3& distance)
{
    Matrix33 d_dot = Matrix33::Identity() * Dot(distance, distance);
    Matrix33 d_outer = Outer(distance, distance);
    return InertiaMatrix(link.inertial.inertia) + link.inertial.mass * (d_dot - d_outer);
}

void compute_accumulated_inertias(const UrdfRobot& robot,
                                  const std::string& joint_name,
                                  Matrix33& backward_accumulated_inertia,
                                  Matrix33& forward_accumulated_inertia)
{
    auto it = robot.joints.find(joint_name);
    if (it == robot.joints.end())
    {
        CARB_LOG_ERROR("Computing Accumulated inertia: Joint not found (%s)", joint_name.c_str());
    }
    const UrdfJoint& joint = it->second;

    struct jointsToVisit
    {
        std::string name;
        Transform forward_position;
        bool is_forward;
    };
    // Reset the accumulated inertias
    backward_accumulated_inertia = Matrix33();
    forward_accumulated_inertia = Matrix33();


    auto accumulate_inertia = [&](std::vector<jointsToVisit>& joints_to_visit, Matrix33& accumulated_inertia)
    {
        std::set<std::string> visited_joints; // keep track of visited joints
        jointsToVisit start_joint = joints_to_visit.back();

        while (!joints_to_visit.empty())
        {
            jointsToVisit current_joint = joints_to_visit.back();
            joints_to_visit.pop_back();
            if (visited_joints.find(current_joint.name) == visited_joints.end())
            {
                // CARB_LOG_WARN("Computing Accumulated inertia:  (%s, %s)", current_joint.name.c_str(),
                //               current_joint.is_forward ? "forward" : "backward");
                visited_joints.insert(current_joint.name);
                auto it = robot.joints.find(current_joint.name);
                if (it != robot.joints.end())
                {
                    const UrdfJoint& this_joint = it->second;
                    const UrdfLink& current_link = current_joint.is_forward ? robot.links.at(this_joint.childLinkName) :
                                                                              robot.links.at(this_joint.parentLinkName);
                    Transform distance;
                    if (current_joint.is_forward)
                    {
                        distance = current_joint.forward_position * this_joint.origin * (current_link.inertial.origin);
                    }
                    else
                    {
                        distance =
                            Inverse(this_joint.origin) * current_joint.forward_position * current_link.inertial.origin;
                    }
                    accumulated_inertia += compute_parallel_axis_inertia(current_link, distance.p);
                    // CARB_LOG_WARN("Accumulated Inertia: %f", L2_magnitude(accumulated_inertia));
                    {
                        if (!current_joint.is_forward)
                        {
                            joints_to_visit.push_back({ this_joint.parentJoint,
                                                        Inverse(this_joint.origin) * current_joint.forward_position,
                                                        false });
                        }
                        if (!(!current_joint.is_forward && current_joint.name == start_joint.name))
                        {
                            for (const std::string& child_joint : this_joint.childrenJoints)
                            {
                                // if (visited_joints.find(child_joint) == visited_joints.end()) {
                                joints_to_visit.push_back(
                                    { child_joint, current_joint.forward_position * this_joint.origin, true });
                                // }
                            }
                        }
                    }
                }
            }
        }
    };

    // CARB_LOG_WARN("Computing Backward Accumulated Inertia start (%s)", joint.name.c_str());
    std::vector<jointsToVisit> joints_to_visit = { { joint.name, Transform(), false } };
    accumulate_inertia(joints_to_visit, backward_accumulated_inertia);
    // CARB_LOG_WARN("Computing Forward Accumulated Inertia start (%s)", joint.name.c_str());
    joints_to_visit = { { joint.name, Transform(), true } };
    accumulate_inertia(joints_to_visit, forward_accumulated_inertia);
}


float computeSimpleStiffness(const UrdfRobot& robot, std::string joint, float naturalFrequency)
{

    // naturalFrequency =  180 / M_PI * naturalFrequency;

    float inertia = 1.0f;
    if (robot.joints.at(joint).drive.driveType == UrdfJointDriveType::FORCE)
    {
        inertia = robot.joints.at(joint).jointInertia;
    }
    // return robot.joints.at(joint).jointInertia * naturalFrequency * naturalFrequency;
    return inertia * naturalFrequency * naturalFrequency;
}

bool parseJoints(const XMLElement& root, std::map<std::string, UrdfJoint>& urdfJoints)
{
    for (auto jointElement = root.FirstChildElement("joint"); jointElement;
         jointElement = jointElement->NextSiblingElement("joint"))
    {
        UrdfJoint joint;

        // name
        auto name = jointElement->Attribute("name");
        if (name)
        {
            joint.name = makeValidUSDIdentifier(name);
        }
        else
        {
            CARB_LOG_ERROR("*** Found unnamed joint ");
            return false;
        }

        auto type = jointElement->Attribute("type");
        if (type)
        {
            if (!parseJointType(type, joint.type))
            {
                return false;
            }
        }
        else
        {
            CARB_LOG_ERROR("*** Found untyped joint ");
            return false;
        }

        auto dontCollapse = jointElement->Attribute("dont_collapse");
        if (dontCollapse)
        {
            joint.dontCollapse = dontCollapse;
        }
        else
        {
            // default: if not specified, collapse the joint
            joint.dontCollapse = false;
        }


        auto parentElement = jointElement->FirstChildElement("parent");
        if (parentElement)
        {
            joint.parentLinkName = makeValidUSDIdentifier(parentElement->Attribute("link"));
        }
        else
        {
            CARB_LOG_ERROR("*** Joint has no parent link ");
            return false;
        }

        auto childElement = jointElement->FirstChildElement("child");
        if (childElement)
        {
            joint.childLinkName = makeValidUSDIdentifier(childElement->Attribute("link"));
        }
        else
        {
            CARB_LOG_ERROR("*** Joint has no child link ");
            return false;
        }

        if (!parseOrigin(*jointElement, joint.origin))
        {
            // optional, default to identity
            joint.origin = Transform();
        }

        if (!parseAxis(*jointElement, joint.axis))
        {
            // optional, default to (1,0,0)
            joint.axis = UrdfAxis();
        }

        if (!parseLimit(*jointElement, joint.limit))
        {
            if (joint.type == UrdfJointType::REVOLUTE || joint.type == UrdfJointType::PRISMATIC)
            {
                CARB_LOG_ERROR("*** limit must be specified for revolute and prismatic ");
                return false;
            }
            joint.limit = UrdfLimit();
        }

        if (!parseDynamics(*jointElement, joint.dynamics))
        {
            // optional
            joint.dynamics = UrdfDynamics();
        }

        urdfJoints.emplace(joint.name, joint);
    }

    // Add second pass to parse mimic information
    for (auto jointElement = root.FirstChildElement("joint"); jointElement;
         jointElement = jointElement->NextSiblingElement("joint"))
    {
        auto name = jointElement->Attribute("name");
        if (name)
        {
            UrdfJoint& joint = urdfJoints[makeValidUSDIdentifier(name)];
            auto mimicElement = jointElement->FirstChildElement("mimic");
            if (mimicElement)
            {
                if (!parseJointMimic(*mimicElement, joint.mimic))
                {
                    joint.mimic.joint = "";
                }
                else
                {
                    auto& parentJoint = urdfJoints[joint.mimic.joint];
                    parentJoint.mimicChildren[joint.name] = joint.mimic.offset;
                }
            }
        }
    }
    return true;
}

bool parseLoopJoints(const XMLElement& element, std::map<std::string, UrdfLoopJoint>& loopJoints)
{
    for (auto jointElement = element.FirstChildElement("loop_joint"); jointElement;
         jointElement = jointElement->NextSiblingElement("loop_joint"))
    {
        UrdfLoopJoint joint;
        const char* link[2] = { "link1", "link2" };
        // name
        auto name = jointElement->Attribute("name");
        if (name)
        {
            CARB_LOG_INFO("Parsing Loop Joint %s", name);
            joint.name = makeValidUSDIdentifier(name);
        }
        else
        {
            CARB_LOG_ERROR("*** Found unnamed joint ");
            return false;
        }

        if (!parseJointType(jointElement->Attribute("type"), joint.type))
        {
            CARB_LOG_WARN("*** Loop Joint %s has no type", joint.name.c_str());
        }
        CARB_LOG_INFO("Joint Type %d", (int)joint.type);

        for (int i = 0; i < 2; i++)
        {
            auto linkElement = jointElement->FirstChildElement(link[i]);
            if (linkElement)
            {
                auto linkName = linkElement->Attribute("link");
                if (linkName)
                {
                    joint.linkName[i] = makeValidUSDIdentifier(linkName);
                }
                else
                {
                    CARB_LOG_ERROR("*** Loop Joint %s has no link pair", joint.name.c_str());
                    return false;
                }
                auto attribute = linkElement->Attribute("xyz");
                Transform origin;
                if (attribute)
                {
                    if (!parseXyz(attribute, origin.p.x, origin.p.y, origin.p.z))
                    {
                        // optional, use zero vector
                        origin.p = Vec3(0.0);
                    }
                }
                attribute = linkElement->Attribute("rpy");
                if (attribute)
                {
                    // parse roll pitch yaw
                    float roll = 0.0f;
                    float pitch = 0.0f;
                    float yaw = 0.0f;
                    if (!parseXyz(attribute, roll, pitch, yaw))
                    {
                        roll = pitch = yaw = 0.0;
                    }
                    // convert to transform quaternion:
                    origin.q = rpy2quat(roll, pitch, yaw);
                }
                joint.linkPose[i] = origin;
            }
            else
            {
                CARB_LOG_ERROR("*** Loop Joint %s has no link pair", joint.name.c_str());
                return false;
            }
        }
        loopJoints.emplace(joint.name, joint);
    }
    return true;
}


bool parseFixedFrames(const XMLElement& element, std::map<std::string, UrdfLink>& links)
{
    for (auto frameElement = element.FirstChildElement("fixed_frame"); frameElement;
         frameElement = frameElement->NextSiblingElement("fixed_frame"))
    {
        // name
        std::string name;
        auto nameattr = frameElement->Attribute("name");
        if (nameattr)
        {
            name = makeValidUSDIdentifier(nameattr);
        }
        else
        {
            CARB_LOG_ERROR("*** Found unnamed fixed frame ");
            return false;
        }
        auto parentElement = frameElement->FirstChildElement("parent");
        std::string parentLink = parentElement->Attribute("link");
        if (parentLink.c_str())
        {
            parentLink = makeValidUSDIdentifier(parentLink);
        }
        else
        {
            CARB_LOG_ERROR("*** Found fixed frame without parent (%s) ", name.c_str());
            return false;
        }
        Transform origin;

        if (!parseOrigin(*frameElement, origin))
        {
            // optional, default to identity
            origin = Transform();
            CARB_LOG_WARN("*** Fixed frame %s has no origin", name.c_str());
        }

        links[parentLink].mergedChildren.emplace(name, origin);
    }
    return true;
}


bool findRootLink(const std::map<std::string, UrdfLink>& urdfLinks,
                  const std::map<std::string, UrdfJoint>& urdfJoints,
                  std::string& rootLinkName)
{
    // Deal with degenerate case where there are no joints
    if (urdfJoints.empty())
    {
        rootLinkName = makeValidUSDIdentifier(urdfLinks.begin()->second.name);
        return true;
    }
    // Create a set to store all child links
    std::unordered_set<std::string> childLinkNames;
    // Iterate through joints and add child links to the set
    for (const auto& joint : urdfJoints)
    {
        childLinkNames.insert(joint.second.childLinkName);
    }
    // Iterate through joints and find the link with no parent
    for (const auto& joint : urdfJoints)
    {
        // If the parent link is not in the set of child links, it is the root link
        if (childLinkNames.find(joint.second.parentLinkName) == childLinkNames.end())
        {
            rootLinkName = makeValidUSDIdentifier(joint.second.parentLinkName);
            // check if root link exist in parsed links
            if (urdfLinks.find(rootLinkName) == urdfLinks.end())
            {
                CARB_LOG_ERROR("*** Root link %s not found in links ", rootLinkName.c_str());
                return false;
            }
            // return true if root link is found
            return true;
        }
    }
    return false;
}

float convertVerticalFovToHorizontal(float verticalFov, const std::pair<int, int>& resolution)
{
    // Calculate the tangent of half the vertical FOV
    float verticalHalfFovTangent = std::tan(verticalFov * M_PI / 360.0 / 2.0);

    // Calculate the horizontal FOV using the aspect ratio of the resolution
    float horizontalHalfFov =
        std::atan(verticalHalfFovTangent * static_cast<float>(resolution.first) / resolution.second);

    // Convert the half horizontal FOV to the full horizontal FOV
    float horizontalFov = 2.0 * horizontalHalfFov * 180.0 / M_PI;

    return horizontalFov;
}

bool parseMujocoCamera(const tinyxml2::XMLElement* element, UrdfCamera& camera)
{
    auto name = element->Attribute("name");
    camera.name = std::string(name);
    parseFloat(element->Attribute("fovy"), camera.hfov);
    parseOrigin(*element, camera.origin);
    camera.origin.q = camera.origin.q * Quat(0, 1, 0, 0);
    camera.clipFar = 1000;
    camera.clipNear = 0.01;
    auto resolution = element->Attribute("resolution");
    if (resolution)
    {
        if (!parseFloatPair(resolution, camera.width, camera.height))
        {
            camera.width = 0;
            camera.height = 0;
        }
    }
    else
    {
        camera.width = 0;
        camera.height = 0;
    }
    camera.updateRate = 30;
    // camera.hfov = convertVerticalFovToHorizontal(camera.hfov, { camera.width, camera.height });
    return true;
}

bool parseCamera(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    auto name = element->Attribute("name");
    CARB_LOG_INFO("Parsing Camera %s", name);
    auto cameraElement = element->FirstChildElement("camera");
    if (cameraElement)
    {

        auto imageElement = cameraElement->FirstChildElement("image");
        UrdfCamera camera;
        camera.name = std::string(name);
        parseOrigin(*element, camera.origin);
        if (!parseFloat(element->Attribute("update_rate"), camera.updateRate))
        {
            camera.updateRate = 30; // Make 30 FPS the default update rate for cameras.
        }
        if (!parseFloat(imageElement->Attribute("width"), camera.width))
        {
            camera.width = 0; // That's a mandatory attribute
        }
        if (!parseFloat(imageElement->Attribute("height"), camera.height))
        {
            camera.height = 0; // That's a mandatory attribute
        }
        if (imageElement->Attribute("format"))
        {
            camera.format = ""; // That's a mandatory attribute
        }
        if (!parseFloat(imageElement->Attribute("near"), camera.clipNear))
        {
            camera.clipNear = 0; // That's a mandatory attribute
        }
        if (!parseFloat(imageElement->Attribute("far"), camera.clipFar))
        {
            camera.clipFar = 1000; // That's a mandatory attribute, but set 1000 meters just to be on the safe side
        }
        if (camera.clipFar < camera.clipNear)
        {
            camera.clipFar = camera.clipNear;
        }
        if (!parseFloat(imageElement->Attribute("hfov"), camera.hfov))
        {
            camera.hfov = 0; // That's a mandatory attribute
        }
        urdfLink.cameras.push_back(camera);
    }
    else
    {
        return false;
    }
    return true;
}

void parseRayDim(const tinyxml2::XMLElement* element, UrdfRayDim& dim)
{
    if (!parseSize(element->Attribute("samples"), dim.samples))
    {
        dim.samples = 0;
    }
    if (!parseFloat(element->Attribute("resolution"), dim.resolution))
    {
        dim.resolution = 0;
    }
    if (!parseFloat(element->Attribute("min_angle"), dim.minAngle))
    {
        dim.minAngle = 0;
    }
    if (!parseFloat(element->Attribute("max_angle"), dim.maxAngle))
    {
        dim.maxAngle = 0;
    }
}

bool parseRay(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    UrdfRay ray;
    ray.name = std::string(element->Attribute("name"));
    parseFloat(element->Attribute("update_rate"), ray.updateRate);
    if (element->Attribute("isaac_sim_config"))
    {
        ray.isaacSimConfig = std::string(element->Attribute("isaac_sim_config"));
    }
    CARB_LOG_INFO("Parsing LIDAR %s", ray.name.c_str());
    parseOrigin(*element, ray.origin);
    auto rayElement = element->FirstChildElement("ray");
    if (rayElement)
    {
        auto horizontal = rayElement->FirstChildElement("horizontal");

        auto vertical = rayElement->FirstChildElement("vertical");

        // auto range = element->FirstChildElement("range");


        // if (range)
        // {
        //     if (!parseFloat(range->Attribute("min"), ray.min))
        //     {
        //         ray.min = 0;
        //     }
        //     if (!parseFloat(range->Attribute("max"), ray.max))
        //     {
        //         ray.max = 0;
        //     }
        //     if (!parseFloat(range->Attribute("resolution"), ray.resolution))
        //     {
        //         ray.resolution = 0;
        //     }
        // }
        if (horizontal)
        {
            parseRayDim(horizontal, ray.horizontal);
            ray.hasHorizontal = true;
        }
        if (vertical)
        {
            parseRayDim(vertical, ray.vertical);
            ray.hasVertical = true;
        }
    }

    urdfLink.lidars.push_back(ray);
    return true;
}
bool parseImu(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseMagnetometer(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseGps(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseForceSensor(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseContact(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseSonar(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseRfidTag(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseRfid(const tinyxml2::XMLElement* element, UrdfLink& urdfLink)
{
    return false;
}
bool parseSensors(const XMLElement& root, std::map<std::string, UrdfLink>& urdfLinks)
{
    // Define a type for function pointers
    using ParseFunction = std::function<bool(const tinyxml2::XMLElement*, UrdfLink&)>;
    static std::vector<ParseFunction> parseFunctions = {
        parseCamera, parseRay,
        //! Other sensors are not officially supported in URDF yet, but leaving skeleton here for future use
        // parseImu,
        // parseMagnetometer,
        // parseGps,
        // parseForceSensor,
        // parseContact,
        // parseSonar,
        // parseRfidTag,
        // parseRfid
    };

    for (auto sensorElement = root.FirstChildElement("sensor"); sensorElement;
         sensorElement = sensorElement->NextSiblingElement("sensor"))
    {
        CARB_LOG_INFO("Parsing Sensor");
        UrdfSensorType type;
        auto parentElement = sensorElement->FirstChildElement("parent");
        if (parentElement)
        {
            auto parent_link = parentElement->Attribute("link");
            auto link = urdfLinks.find(parent_link);
            if (link != urdfLinks.end())
            {
                if (parseSensorType(*sensorElement, type))
                {

                    if (!parseFunctions[static_cast<int>(type)](sensorElement, link->second))
                    {
                        auto name = sensorElement->Attribute("name");
                        CARB_LOG_ERROR("Error parsing sensor %s.", name);
                    }
                }
                else
                {
                    auto name = sensorElement->Attribute("name");
                    auto type = sensorElement->Attribute("type");
                    CARB_LOG_WARN("Sensor %s not parsed: Sensor type unsupported (%s)", name, type);
                }
            }
        }
    }
    for (auto mujocoCameraElement = root.FirstChildElement("mujoco_camera"); mujocoCameraElement;
         mujocoCameraElement = mujocoCameraElement->NextSiblingElement("mujoco_camera"))
    {
        UrdfCamera camera;
        if (parseMujocoCamera(mujocoCameraElement, camera))
        {
            auto parentElement = mujocoCameraElement->FirstChildElement("parent");
            if (parentElement)
            {
                auto parent_link = parentElement->Attribute("link");
                auto link = urdfLinks.find(parent_link);
                if (link != urdfLinks.end())
                {
                    link->second.cameras.push_back(camera);
                }
            }
        }
    }
    return true;
}

// bool parseSpringGroup(const XMLElement& element, UrdfSpringGroup& springGroup)
// {
//     auto start = element.Attribute("start");
//     auto count = element.Attribute("size");
//     auto scale = element.Attribute("scale");
//     if (!start || !parseInt(start, springGroup.springStart))
//     {
//         return false;
//     }
//     if (!count || !parseInt(count, springGroup.springCount))
//     {
//         return false;
//     }
//     if (!scale || !parseFloat(scale, springGroup.scale))
//     {
//         return false;
//     }
//     return true;
// }

// bool parseSoftActuators(const XMLElement& root, std::vector<UrdfSoft1DActuator>& urdfActs)
// {
//     auto actuatorElement = root.FirstChildElement("actuator");
//     while (actuatorElement)
//     {
//         auto name = actuatorElement->Attribute("name");
//         auto type = actuatorElement->Attribute("type");
//         if (type)
//         {
//             if (strcmp(type, "pneumatic1D") == 0)
//             {
//                 UrdfSoft1DActuator act;
//                 act.name = std::string(name);
//                 act.link = std::string(actuatorElement->Attribute("link"));
//                 auto gains = actuatorElement->FirstChildElement("gains");
//                 auto thresholds = actuatorElement->FirstChildElement("thresholds");
//                 auto springGroups = actuatorElement->FirstChildElement("springGroups");
//                 auto maxPressure = actuatorElement->Attribute("maxPressure");
//                 if (!maxPressure || !parseFloat(maxPressure, act.maxPressure))
//                 {
//                     act.maxPressure = 0.0f;
//                 }
//                 if (gains)
//                 {
//                     auto inflate = gains->Attribute("inflate");
//                     auto deflate = gains->Attribute("deflate");
//                     if (!inflate || !parseFloat(inflate, act.inflateGain))
//                     {
//                         act.inflateGain = 1.0f;
//                     }
//                     if (!deflate || !parseFloat(deflate, act.deflateGain))
//                     {
//                         act.deflateGain = 1.0f;
//                     }
//                 }
//                 if (thresholds)
//                 {
//                     auto deflate = thresholds->Attribute("deflate");
//                     auto deactivate = thresholds->Attribute("deactivate");
//                     if (!deflate || !parseFloat(deflate, act.deflateThreshold))
//                     {
//                         act.deflateThreshold = 1.0f;
//                     }
//                     if (!deactivate || !parseFloat(deactivate, act.deactivateThreshold))
//                     {
//                         act.deactivateThreshold = 1.0e-2f;
//                     }
//                 }
//                 if (springGroups)
//                 {
//                     auto springGroup = springGroups->FirstChildElement("springGroup");
//                     while (springGroup)
//                     {
//                         UrdfSpringGroup sg;
//                         if (parseSpringGroup(*springGroup, sg))
//                         {
//                             act.springGroups.push_back(sg);
//                         }
//                         springGroup = springGroup->NextSiblingElement("springGroup");
//                     }
//                 }
//                 if (maxPressure && gains && thresholds && springGroups)
//                     urdfActs.push_back(act);
//                 else
//                 {
//                     CARB_LOG_ERROR("*** unable to parse soft actuator %s ", act.name.c_str());
//                     return false;
//                 }
//             }
//         }
//         actuatorElement = actuatorElement->NextSiblingElement("actuator");
//     }
//     return true;
// }

void populateJointTree(isaacsim::asset::importer::urdf::UrdfRobot& robot)
{
    // CARB_LOG_WARN("Populating Joint Tree");
    std::map<std::string, std::string> jointChildMap; // Since URDF is a tree, each link should be listed only once as a
                                                      // ChildLink by a joint
    // Pre-populate the joint to child link map.
    for (auto& joint : robot.joints)
    {
        // CARB_LOG_WARN("Joint: %s, Child Link: %s", joint.second.name.c_str(), joint.second.childLinkName.c_str());
        jointChildMap.emplace(joint.second.childLinkName, joint.second.name);
    }

    for (auto& joint : robot.joints)
    {
        // CARB_LOG_WARN("Joint: %s, Parent Link: %s", joint.second.name.c_str(), joint.second.parentLinkName.c_str());
        auto parent_joint = jointChildMap.find(joint.second.parentLinkName);
        joint.second.parentJoint = "";
        if (parent_joint != jointChildMap.end())
        {
            joint.second.parentJoint = parent_joint->second;
            auto& parent_joint = robot.joints.at(joint.second.parentJoint);
            parent_joint.childrenJoints.push_back(joint.second.name);
        }
        // CARB_LOG_WARN("Parent Joint: %s", joint.second.parentJoint.c_str());
    }

    for (auto& joint : robot.joints)
    {
        Matrix33 parent_inertia;
        Matrix33 child_inertia;

        compute_accumulated_inertias(robot, joint.second.name, parent_inertia, child_inertia);

        float m0 = L2_magnitude(parent_inertia);
        float m1 = L2_magnitude(child_inertia);

        float mass_eq = computeEquivalentInertia(m0, m1);
        // CARB_LOG_WARN("Joint: %s, Equivalent Inertia: %f", joint.second.name.c_str(), mass_eq);
        joint.second.jointInertia = mass_eq;
    }
}

bool parseRobot(const XMLElement& root, UrdfRobot& urdfRobot)
{
    auto name = root.Attribute("name");
    if (name)
    {
        urdfRobot.name = makeValidUSDIdentifier(name);
    }

    urdfRobot.links.clear();
    urdfRobot.joints.clear();
    urdfRobot.materials.clear();

    if (!parseMaterials(root, urdfRobot.materials))
    {
        return false;
    }
    if (!parseLinks(root, urdfRobot.links))
    {
        return false;
    }
    if (!parseJoints(root, urdfRobot.joints))
    {
        return false;
    }

    if (!parseLoopJoints(root, urdfRobot.loopJoints))
    {
        CARB_LOG_WARN(
            "Error parsing loop joints - please check your import results for inacuracies and if the loop joints are correctly parsed");
    }

    if (!parseFixedFrames(root, urdfRobot.links))
    {
        CARB_LOG_WARN(
            "Error parsing fixed frames - please check your import results for inacuracies and if the fixed frames are correctly parsed");
    }
    // CARB_LOG_WARN("Populating Joint Tree");
    populateJointTree(urdfRobot);

    if (!parseSensors(root, urdfRobot.links))
    {
    }
    if (!findRootLink(urdfRobot.links, urdfRobot.joints, urdfRobot.rootLink))
    {
        return false;
    }
    // if (!parseSoftActuators(root, urdfRobot.softActuators))
    // {
    //     return false;
    // }
    return true;
}

bool parseUrdf(const std::string& urdfPackagePath, const std::string& urdfFileRelativeToPackage, UrdfRobot& urdfRobot)
{
    std::string path;

    //    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    //    path = GetFilePathByPlatform((urdfPackagePath + "\\" + urdfFileRelativeToPackage).c_str());
    // #else
    //    path = GetFilePathByPlatform((urdfPackagePath + "/" + urdfFileRelativeToPackage).c_str());
    // #endif

    path = urdfPackagePath + "/" + urdfFileRelativeToPackage;

    CARB_LOG_INFO("Loading URDF at '%s'", path.c_str());

    // Weird stack smashing error with tinyxml2 when the descructor is called
    static tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
    {
        CARB_LOG_ERROR("*** Failed to load '%s'", path.c_str());
        return false;
    }

    XMLElement* root = doc.RootElement();
    if (!root)
    {
        CARB_LOG_ERROR("*** Empty document '%s' ", path.c_str());
        return false;
    }

    if (!parseRobot(*root, urdfRobot))
    {
        return false;
    }

    // std::cout << urdfRobot << std::endl;

    return true;
}

bool parseUrdfString(const std::string& urdf_str, UrdfRobot& urdfRobot)
{

    //    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    //    path = GetFilePathByPlatform((urdfPackagePath + "\\" + urdfFileRelativeToPackage).c_str());
    // #else
    //    path = GetFilePathByPlatform((urdfPackagePath + "/" + urdfFileRelativeToPackage).c_str());
    // #endif

    CARB_LOG_INFO("Loading URDF from memory");
    CARB_LOG_INFO("%s", urdf_str.c_str());
    // Weird stack smashing error with tinyxml2 when the descructor is called
    static tinyxml2::XMLDocument doc;
    if (doc.Parse(urdf_str.c_str()) != XML_SUCCESS)
    {
        CARB_LOG_ERROR("*** Failed to load '%s'", urdf_str.c_str());
        return false;
    }

    XMLElement* root = doc.RootElement();
    if (!root)
    {
        CARB_LOG_ERROR("*** Empty document '%s' ", urdf_str.c_str());
        return false;
    }

    if (!parseRobot(*root, urdfRobot))
    {
        return false;
    }

    // std::cout << urdfRobot << std::endl;

    return true;
}

} // namespace urdf
}
}
}
