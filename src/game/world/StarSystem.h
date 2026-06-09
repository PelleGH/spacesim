#pragma once

#include <cmath>
#include <string>
#include <vector>

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

    struct OrbitData
    {
        int parentIndex = -1;
        double radius = 0.0;
        double angle = 0.0;
        double angularSpeed = 0.0;
    };

    struct GlobalObject
    {
        std::string name;
        GlobalObjectType type = GlobalObjectType::Planet;

        DVec3 position{};

        double visualRadius = 1.0;

        bool hasOrbit = false;
        OrbitData orbit{};

        bool isJumpTarget = false;
    };

    struct StarSystem
    {
        std::vector<GlobalObject> objects;
    };
    
    inline StarSystem CreateTestStarSystem()
    {
        StarSystem system;

        system.objects.push_back(GlobalObject{
            .name = "Helios",
            .type = GlobalObjectType::Sun,
            .position = { 0.0, 0.0, 0.0 },
            .visualRadius = 50000.0
        });

        system.objects.push_back(GlobalObject{
            .name = "Aster",
            .type = GlobalObjectType::Planet,
            .position = { 300000.0, 0.0, 0.0 },
            .visualRadius = 12000.0
        });

        system.objects.push_back(GlobalObject{
            .name = "Boreal",
            .type = GlobalObjectType::Planet,
            .position = { -700000.0, 0.0, 300000.0 },
            .visualRadius = 18000.0
        });

        system.objects.push_back(GlobalObject{
            .name = "Cyra",
            .type = GlobalObjectType::Planet,
            .position = { 200000.0, 0.0, -1200000.0 },
            .visualRadius = 9000.0
        });

        system.objects.push_back(GlobalObject{
            .name = "Aster Relay",
            .type = GlobalObjectType::Satellite,
            .position = { 345000.0, 0.0, 0.0 },
            .visualRadius = 500.0,
            .isJumpTarget = true
        });

        system.objects.push_back(GlobalObject{
            .name = "Boreal Relay",
            .type = GlobalObjectType::Satellite,
            .position = { -700000.0, 0.0, 360000.0 },
            .visualRadius = 500.0,
            .isJumpTarget = true
        });

        system.objects.push_back(GlobalObject{
            .name = "Cyra Relay",
            .type = GlobalObjectType::Satellite,
            .position = { 200000.0, 0.0, -1165000.0 },
            .visualRadius = 500.0,
            .isJumpTarget = true
        });

        return system;
    }
}