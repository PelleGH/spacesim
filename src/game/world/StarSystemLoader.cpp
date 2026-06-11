#include "world/StarSystemLoader.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace SpaceSim
{
    static PlanetClass ReadPlanetClass(const std::string& value)
    {
        if (value == "rocky") return PlanetClass::Rocky;
        if (value == "barren_moon") return PlanetClass::BarrenMoon;
        if (value == "ice_world") return PlanetClass::IceWorld;
        if (value == "desert_world") return PlanetClass::DesertWorld;
        if (value == "ocean_world") return PlanetClass::OceanWorld;
        if (value == "lava_world") return PlanetClass::LavaWorld;
        if (value == "gas_giant") return PlanetClass::GasGiant;
        if (value == "ice_giant") return PlanetClass::IceGiant;
        if (value == "carbon_world") return PlanetClass::CarbonWorld;

        return PlanetClass::Rocky;
    }

    static PlanetComposition ReadPlanetComposition(const std::string& value)
    {
        if (value == "silicate_iron") return PlanetComposition::SilicateIron;
        if (value == "ice_rock") return PlanetComposition::IceRock;
        if (value == "carbon_rich") return PlanetComposition::CarbonRich;
        if (value == "metallic") return PlanetComposition::Metallic;
        if (value == "hydrogen_helium") return PlanetComposition::HydrogenHelium;
        if (value == "methane_ammonia") return PlanetComposition::MethaneAmmonia;
        if (value == "sulfuric") return PlanetComposition::Sulfuric;

        return PlanetComposition::Unknown;
    }

    static AtmosphereType ReadAtmosphereType(const std::string& value)
    {
        if (value == "none") return AtmosphereType::None;
        if (value == "thin") return AtmosphereType::Thin;
        if (value == "breathable") return AtmosphereType::Breathable;
        if (value == "thick") return AtmosphereType::Thick;
        if (value == "toxic") return AtmosphereType::Toxic;
        if (value == "hydrogen_helium") return AtmosphereType::HydrogenHelium;

        return AtmosphereType::None;
    }

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
            if (objectJson.contains("planet"))
            {
                const auto& planetJson = objectJson.at("planet");

                object.hasPlanetData = true;

                object.planetData.planetClass = ReadPlanetClass(
                    planetJson.value("class", "rocky")
                );

                object.planetData.composition = ReadPlanetComposition(
                    planetJson.value("composition", "silicate_iron")
                );

                object.planetData.atmosphere = ReadAtmosphereType(
                    planetJson.value("atmosphere", "none")
                );

                object.planetData.temperatureK = planetJson.value("temperatureK", 280.0);
                object.planetData.seed = planetJson.value("seed", 1u);

                if (planetJson.contains("visualOverrides"))
                {
                    const auto& overrides = planetJson.at("visualOverrides");

                    if (overrides.contains("baseColor"))
                    {
                        object.planetData.visualOverrides.baseColor =
                            ReadColor(overrides.at("baseColor"));
                    }

                    if (overrides.contains("secondaryColor"))
                    {
                        object.planetData.visualOverrides.secondaryColor =
                            ReadColor(overrides.at("secondaryColor"));
                    }

                    if (overrides.contains("atmosphereColor"))
                    {
                        object.planetData.visualOverrides.atmosphereColor =
                            ReadColor(overrides.at("atmosphereColor"));
                    }
                }
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