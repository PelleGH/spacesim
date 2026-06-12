#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <raylib.h>
namespace SpaceSim
{
    struct DVec3
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    inline DVec3 operator+(DVec3 a, DVec3 b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    inline DVec3 operator-(DVec3 a, DVec3 b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    inline DVec3 operator*(DVec3 a, double scalar)
    {
        return { a.x * scalar, a.y * scalar, a.z * scalar };
    }

    inline double Length(DVec3 v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    inline DVec3 Normalize(DVec3 v)
    {
        const double length = Length(v);

        if (length <= 0.000001)
        {
            return { 0.0, 0.0, 1.0 };
        }

        return {
            v.x / length,
            v.y / length,
            v.z / length
        };
    }

    enum class GlobalObjectType
    {
        Sun,
        Planet,
        Satellite
    };

    enum class PlanetClass
    {
        Ocean,
        Rocky,
        Ice,
        Desert,
        Barren
    };
    struct OrbitData
    {
        int parentIndex = -1;
        double radius = 0.0;
        double angle = 0.0;
        double angularSpeed = 0.0;
    };

    struct GlobalObject
    {
        std::string id;
        std::string name;

        GlobalObjectType type = GlobalObjectType::Planet;

        DVec3 position{};

        double visualRadius = 1.0;

        Color color = WHITE;

        PlanetClass planetClass = PlanetClass::Rocky;
        
        std::string parentId;
        int parentIndex = -1;

        bool hasOrbit = false;
        OrbitData orbit{};

        bool isJumpTarget = false;
    };

    struct StarSystem
    {
        std::string name;
        std::vector<GlobalObject> objects;
    };
}