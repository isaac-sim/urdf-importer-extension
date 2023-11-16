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

#include "UrdfParser.h"

#include "../core/PathUtils.h"

#include <carb/logging/Log.h>

namespace omni
{
namespace importer
{
namespace urdf
{

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
    if (sscanf(str, "%f %f %f", &x, &y, &z) == 3)
    {
        return true;
    }
    else
    {
        printf("*** Could not parse xyz string '%s' \n", str);
        return false;
    }
}

bool parseColor(const char* str, float& r, float& g, float& b, float& a)
{
    if (sscanf(str, "%f %f %f %f", &r, &g, &b, &a) == 4)
    {
        return true;
    }
    else
    {
        printf("*** Could not parse color string '%s' \n", str);
        return false;
    }
}

bool parseFloat(const char* str, float& value)
{
    if (sscanf(str, "%f", &value) == 1)
    {
        return true;
    }
    else
    {
        printf("*** Could not parse float string '%s' \n", str);
        return false;
    }
}

bool parseInt(const char* str, int& value)
{
    if (sscanf(str, "%d", &value) == 1)
    {
        return true;
    }
    else
    {
        printf("*** Could not parse int string '%s' \n", str);
        return false;
    }
}

bool parseJointType(const char* str, UrdfJointType& type)
{
    if (strcmp(str, "revolute") == 0)
    {
        type = UrdfJointType::REVOLUTE;
    }
    else if (strcmp(str, "continuous") == 0)
    {
        type = UrdfJointType::CONTINUOUS;
    }
    else if (strcmp(str, "prismatic") == 0)
    {
        type = UrdfJointType::PRISMATIC;
    }
    else if (strcmp(str, "fixed") == 0)
    {
        type = UrdfJointType::FIXED;
    }
    else if (strcmp(str, "floating") == 0)
    {
        type = UrdfJointType::FLOATING;
    }
    else if (strcmp(str, "planar") == 0)
    {
        type = UrdfJointType::PLANAR;
    }
    else
    {
        printf("*** Unknown joint type '%s' \n", str);
        return false;
    }
    return true;
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
    if (!offsetElement ||!parseFloat(offsetElement, mimic.offset))
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
                printf("*** xyz not specified for axis\n");
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
                printf("*** effort not specified for limit\n");
                return false;
            }
        }
        attribute = limitElement->Attribute("velocity");
        if (attribute)
        {
            if (!parseFloat(attribute, limit.velocity))
            {
                printf("*** velocity not specified for limit\n");
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
                dynamics.damping = 0;
            }
        }
        attribute = dynamicsElement->Attribute("friction");
        if (attribute)
        {
            if (!parseFloat(attribute, dynamics.friction))
            {
                // optional
                dynamics.friction = 0;
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
                printf("*** couldn't parse mass \n");
                return false;
            }
            return true;
        }
        else
        {
            printf("*** mass missing from inertia \n");
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
            printf("*** ixx missing from inertia \n");
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
            printf("*** ixz missing from inertia \n");
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
            printf("*** ixy missing from inertia \n");
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
            printf("*** iyy missing from inertia \n");
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
            printf("*** iyz missing from inertia \n");
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
            printf("*** izz missing from inertia \n");
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
                printf("*** mesh geometry requires a file path \n");
                return false;
            }

            auto scale = geometryElementChild->Attribute("scale");
            if (scale)
            {
                if (!parseXyz(scale, geometry.scale_x, geometry.scale_y, geometry.scale_z))
                {
                    printf("*** scale is missing xyz \n");
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
                    printf("*** couldn't parse xyz size \n");
                    return false;
                }
            }
            else
            {
                printf("*** box geometry requires a size \n");
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
                    printf("*** couldn't parse radius \n");
                    return false;
                }
            }
            else
            {
                printf("*** cylinder geometry requires a radius \n");
                return false;
            }

            attribute = geometryElementChild->Attribute("length");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.length))
                {
                    printf("*** couldn't parse length \n");
                    return false;
                }
            }
            else
            {
                printf("*** cylinder geometry requires a length \n");
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
                    printf("*** couldn't parse radius \n");
                    return false;
                }
            }
            else
            {
                printf("*** capsule geometry requires a radius \n");
                return false;
            }

            attribute = geometryElementChild->Attribute("length");
            if (attribute)
            {
                if (!parseFloat(attribute, geometry.length))
                {
                    printf("*** couldn't parse length \n");
                    return false;
                }
            }
            else
            {
                printf("*** capsule geometry requires a length \n");
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
                    printf("*** couldn't parse radius \n");
                    return false;
                }
            }
            else
            {
                printf("*** sphere geometry requires a radius \n");
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
                    printf("*** filename required for material with texture \n");
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
                printf("*** Found unnamed material \n");
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
                    printf("*** filename required for material with texture \n");
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
                printf("*** Found unnamed link \n");
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
                        printf("*** Found visual without geometry \n");
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
                        printf("*** Found collision without geometry \n");
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

bool parseJoints(const XMLElement& root, std::map<std::string, UrdfJoint>& urdfJoints)
{
     for(auto jointElement = root.FirstChildElement("joint"); jointElement; jointElement = jointElement->NextSiblingElement("joint")) {
        UrdfJoint joint;

        // name
        auto name = jointElement->Attribute("name");
        if (name)
        {
            joint.name = makeValidUSDIdentifier(name);
        }
        else
        {
            printf("*** Found unnamed joint \n");
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
            printf("*** Found untyped joint \n");
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
            printf("*** Joint has no parent link \n");
            return false;
        }

        auto childElement = jointElement->FirstChildElement("child");
        if (childElement)
        {
            joint.childLinkName = makeValidUSDIdentifier(childElement->Attribute("link"));
        }
        else
        {
            printf("*** Joint has no child link \n");
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
                printf("*** limit must be specified for revolute and prismatic \n");
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
    for(auto jointElement = root.FirstChildElement("joint"); jointElement; jointElement = jointElement->NextSiblingElement("joint"))
    {
        auto name = jointElement->Attribute("name");
        if (name)
        {
            UrdfJoint& joint = urdfJoints[makeValidUSDIdentifier(name)];
            auto mimicElement = jointElement->FirstChildElement("mimic");
            if (mimicElement)
            {
                if(!parseJointMimic(*mimicElement, joint.mimic))
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


bool findRootLink(const std::map<std::string, UrdfLink>& urdfLinks, const std::map<std::string, UrdfJoint>& urdfJoints, std::string& rootLinkName)
{
    // Deal with degenerate case where there are no joints
    if (urdfJoints.empty()) {
        rootLinkName = makeValidUSDIdentifier(urdfLinks.begin()->second.name);
        return true;
    }
    // Create a set to store all child links
    std::unordered_set<std::string> childLinkNames;
    // Iterate through joints and add child links to the set
    for (const auto& joint : urdfJoints) {
        childLinkNames.insert(joint.second.childLinkName);
    }
    // Iterate through joints and find the link with no parent
    for (const auto& joint : urdfJoints) {
        // If the parent link is not in the set of child links, it is the root link
        if (childLinkNames.find(joint.second.parentLinkName) == childLinkNames.end()) {
            rootLinkName = makeValidUSDIdentifier(joint.second.parentLinkName);
            // check if root link exist in parsed links
            if (urdfLinks.find(rootLinkName) == urdfLinks.end()) {
                printf("*** Root link %s not found in links \n", rootLinkName.c_str());
                return false;
            }
            // return true if root link is found
            return true;
        }
    }
    return false;
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
//                     printf("*** unable to parse soft actuator %s \n", act.name.c_str());
//                     return false;
//                 }
//             }
//         }
//         actuatorElement = actuatorElement->NextSiblingElement("actuator");
//     }
//     return true;
// }


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
    //#else
    //    path = GetFilePathByPlatform((urdfPackagePath + "/" + urdfFileRelativeToPackage).c_str());
    //#endif

    path = urdfPackagePath + "/" + urdfFileRelativeToPackage;

    CARB_LOG_INFO("Loading URDF at '%s'", path.c_str());

    // Weird stack smashing error with tinyxml2 when the descructor is called
    static tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
    {
        printf("*** Failed to load '%s'", path.c_str());
        return false;
    }

    XMLElement* root = doc.RootElement();
    if (!root)
    {
        printf("*** Empty document '%s' \n", path.c_str());
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
