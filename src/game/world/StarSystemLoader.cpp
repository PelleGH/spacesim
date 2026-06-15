#include "world/StarSystemLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace SpaceSim
{
    static DVec3 ReadDVec3(const nlohmann::json& json)
    {
        return DVec3{
            json.at(0).get<double>(),
            json.at(1).get<double>(),
            json.at(2).get<double>()
        };
    }

    static Color ReadColor(const nlohmann::json& json)
    {
        return Color{
            json.at(0).get<unsigned char>(),
            json.at(1).get<unsigned char>(),
            json.at(2).get<unsigned char>(),
            json.at(3).get<unsigned char>()
        };
    }

    static GlobalObjectType ReadObjectType(const std::string& type)
    {
        if (type == "sun")
        {
            return GlobalObjectType::Sun;
        }

        if (type == "planet")
        {
            return GlobalObjectType::Planet;
        }

        if (type == "satellite")
        {
            return GlobalObjectType::Satellite;
        }

        throw std::runtime_error("Unknown global object type: " + type);
    }
    static PlanetClass ReadPlanetClass(const std::string& planetClass)
    {
        if (planetClass == "earthlike")
        {
            return PlanetClass::EarthLike;
        }

        if (planetClass == "desert")
        {
            return PlanetClass::Desert;
        }

        if (planetClass == "ice")
        {
            return PlanetClass::Ice;
        }

        if (planetClass == "barren")
        {
            return PlanetClass::Barren;
        }

        if (planetClass == "ocean")
        {
            return PlanetClass::Ocean;
        }

        if (planetClass == "gas_giant")
        {
            return PlanetClass::GasGiant;
        }

        throw std::runtime_error("Unknown planet class: " + planetClass);
    }
    StarSystem LoadStarSystemFromJson(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open star system file: " + path);
        }

        nlohmann::json root;
        file >> root;

        StarSystem system;
        system.name = root.value("name", "Unnamed System");

        const auto& objects = root.at("objects");

        for (const auto& objectJson : objects)
        {
            GlobalObject object;

            object.id = objectJson.at("id").get<std::string>();
            object.name = objectJson.at("name").get<std::string>();
            object.type = ReadObjectType(objectJson.at("type").get<std::string>());
            if (object.type == GlobalObjectType::Planet)
            {
                object.planetClass = ReadPlanetClass(
                    objectJson.value("planetClass", "earthlike")
                );
            }
            object.position = ReadDVec3(objectJson.at("position"));
            object.visualRadius = objectJson.value("visualRadius", 1.0);
            object.color = objectJson.contains("color")
                ? ReadColor(objectJson.at("color"))
                : WHITE;
            object.parentId = objectJson.value("parentId", "");
            object.isJumpTarget = objectJson.value("isJumpTarget", false);

            system.objects.push_back(object);
        }

        std::unordered_map<std::string, int> indexById;

        for (int i = 0; i < static_cast<int>(system.objects.size()); ++i)
        {
            indexById[system.objects[i].id] = i;
        }

        for (auto& object : system.objects)
        {
            if (!object.parentId.empty())
            {
                auto found = indexById.find(object.parentId);

                if (found != indexById.end())
                {
                    object.parentIndex = found->second;
                }
            }
        }

        return system;
    }
}