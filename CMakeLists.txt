cmake_minimum_required(VERSION 3.24)

project(SpaceSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(raylib CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(SpaceSim
    src/main.cpp

    src/engine/core/Application.cpp
    src/engine/core/Application.h

    src/engine/renderer/Renderer.cpp
    src/engine/renderer/Renderer.h

    src/game/world/GameWorld.h

    src/game/components/TransformComponent.h
    src/game/components/ShipFlightComponent.h
    src/game/components/PlayerControlledComponent.h

    src/game/systems/ShipControlSystem.cpp
    src/game/systems/ShipControlSystem.h

    src/game/systems/CameraSystem.cpp
    src/game/systems/CameraSystem.h

    src/game/systems/RenderSystem.cpp
    src/game/systems/RenderSystem.h
    src/game/systems/BubbleSystem.cpp
    src/game/systems/BubbleSystem.h

    src/game/world/StarSystem.h

    src/game/systems/StarSystemSystem.cpp
    src/game/systems/StarSystemSystem.h

    src/game/systems/FTLSystem.cpp
    src/game/systems/FTLSystem.h

    src/game/world/StarSystemLoader.cpp
    src/game/world/StarSystemLoader.h
    src/game/world/PlanetVisualGenerator.cpp
    src/game/world/PlanetTerrainGenerator.cpp
    

    src/game/rendering/SpaceBackgroundRenderer.cpp
    src/game/rendering/DistantBodyRenderer.cpp
    src/game/rendering/PrototypeMeshRenderer.cpp
    src/game/rendering/PlanetMeshGenerator.cpp
    src/game/rendering/PlanetCloudGenerator.cpp
)

target_include_directories(SpaceSim PRIVATE
    src
    src/engine
    src/game
)

target_link_libraries(SpaceSim PRIVATE
    raylib
    EnTT::EnTT
    nlohmann_json::nlohmann_json
)