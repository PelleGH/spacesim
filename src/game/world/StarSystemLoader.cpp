#include "world/StarSystemLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>
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
    static PlanetClass ParsePlanetClass(const std::string& value)
    {
        if (value == "ocean") return PlanetClass::Ocean;
        if (value == "rocky") return PlanetClass::Rocky;
        if (value == "ice") return PlanetClass::Ice;
        if (value == "desert") return PlanetClass::Desert;
        if (value == "barren") return PlanetClass::Barren;

        return PlanetClass::Rocky;
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
            object.position = ReadDVec3(objectJson.at("position"));
            object.visualRadius = objectJson.value("visualRadius", 1.0);
            object.color = objectJson.contains("color")
                ? ReadColor(objectJson.at("color"))
                : WHITE;
            if (objectJson.contains("planetClass"))
            {
                object.planetClass = ParsePlanetClass(
                    objectJson.value("planetClass", "rocky")
                );
            }
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