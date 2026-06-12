#include "rendering/PlanetMeshGenerator.h"

#include "world/PlanetTerrainGenerator.h"

#include <raymath.h>
#include <rlgl.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        struct VertexData
        {
            Vector3 position{};
            Vector3 normal{};
            Color color = WHITE;
        };

        struct FaceBasis
        {
            Vector3 normal{};
            Vector3 axisA{};
            Vector3 axisB{};
        };

        constexpr std::uint32_t PlanetMeshCacheVersion = 2u;

        struct MeshCacheHeader
        {
            char magic[8]{ 'S', 'P', 'M', 'E', 'S', 'H', '0', '1' };
            std::uint32_t version = PlanetMeshCacheVersion;
            std::uint32_t vertexCount = 0u;
            std::uint32_t indexCount = 0u;
        };

        static std::string SanitizeCachePart(const std::string& value)
        {
            std::string result;
            result.reserve(value.size());

            for (char c : value)
            {
                const bool valid =
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    c == '_' ||
                    c == '-';

                result.push_back(valid ? c : '_');
            }

            if (result.empty())
            {
                return "object";
            }

            return result;
        }

        static std::filesystem::path GetPlanetMeshCachePath(
            const GlobalObject& object,
            int faceResolution
        )
        {
            // Version is intentionally part of the filename.
            // Increase PlanetMeshCacheVersion when terrain/color generation changes.
            std::string filename =
                "planetmesh_v" + std::to_string(PlanetMeshCacheVersion) +
                "_r" + std::to_string(faceResolution) +
                "_" + SanitizeCachePart(object.id);

            if (object.hasPlanetData)
            {
                filename +=
                    "_c" + std::to_string(static_cast<int>(object.planetData.planetClass)) +
                    "_comp" + std::to_string(static_cast<int>(object.planetData.composition)) +
                    "_atm" + std::to_string(static_cast<int>(object.planetData.atmosphere)) +
                    "_seed" + std::to_string(object.planetData.seed);
            }

            filename += ".bin";

            return std::filesystem::path("cache") / "planet_meshes" / filename;
        }

        static bool LoadPlanetMeshCache(
            const std::filesystem::path& path,
            std::vector<VertexData>& vertices,
            std::vector<unsigned short>& indices
        )
        {
            std::ifstream file(path, std::ios::binary);
            if (!file)
            {
                return false;
            }

            MeshCacheHeader header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            const std::array<char, 8> expectedMagic{
                'S', 'P', 'M', 'E', 'S', 'H', '0', '1'
            };

            if (!file ||
                std::array<char, 8>{
                    header.magic[0], header.magic[1], header.magic[2], header.magic[3],
                    header.magic[4], header.magic[5], header.magic[6], header.magic[7]
                } != expectedMagic ||
                header.version != PlanetMeshCacheVersion ||
                header.vertexCount == 0u ||
                header.indexCount == 0u)
            {
                return false;
            }

            vertices.resize(static_cast<std::size_t>(header.vertexCount));
            indices.resize(static_cast<std::size_t>(header.indexCount));

            file.read(
                reinterpret_cast<char*>(vertices.data()),
                static_cast<std::streamsize>(vertices.size() * sizeof(VertexData))
            );

            file.read(
                reinterpret_cast<char*>(indices.data()),
                static_cast<std::streamsize>(indices.size() * sizeof(unsigned short))
            );

            return file.good();
        }

        static void SavePlanetMeshCache(
            const std::filesystem::path& path,
            const std::vector<VertexData>& vertices,
            const std::vector<unsigned short>& indices
        )
        {
            std::error_code error;
            std::filesystem::create_directories(path.parent_path(), error);

            if (error)
            {
                return;
            }

            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                return;
            }

            MeshCacheHeader header{};
            header.vertexCount = static_cast<std::uint32_t>(vertices.size());
            header.indexCount = static_cast<std::uint32_t>(indices.size());

            file.write(reinterpret_cast<const char*>(&header), sizeof(header));
            file.write(
                reinterpret_cast<const char*>(vertices.data()),
                static_cast<std::streamsize>(vertices.size() * sizeof(VertexData))
            );
            file.write(
                reinterpret_cast<const char*>(indices.data()),
                static_cast<std::streamsize>(indices.size() * sizeof(unsigned short))
            );
        }


        static FaceBasis GetFaceBasis(int face)
        {
            switch (face)
            {
            case 0: // +X
                return FaceBasis{
                    Vector3{ 1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 0.0f, -1.0f },
                    Vector3{ 0.0f, 1.0f, 0.0f }
                };

            case 1: // -X
                return FaceBasis{
                    Vector3{ -1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 0.0f, 1.0f },
                    Vector3{ 0.0f, 1.0f, 0.0f }
                };

            case 2: // +Y
                return FaceBasis{
                    Vector3{ 0.0f, 1.0f, 0.0f },
                    Vector3{ 1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 0.0f, -1.0f }
                };

            case 3: // -Y
                return FaceBasis{
                    Vector3{ 0.0f, -1.0f, 0.0f },
                    Vector3{ 1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 0.0f, 1.0f }
                };

            case 4: // +Z
                return FaceBasis{
                    Vector3{ 0.0f, 0.0f, 1.0f },
                    Vector3{ 1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 1.0f, 0.0f }
                };

            case 5: // -Z
                return FaceBasis{
                    Vector3{ 0.0f, 0.0f, -1.0f },
                    Vector3{ -1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 1.0f, 0.0f }
                };

            default:
                return FaceBasis{
                    Vector3{ 0.0f, 0.0f, 1.0f },
                    Vector3{ 1.0f, 0.0f, 0.0f },
                    Vector3{ 0.0f, 1.0f, 0.0f }
                };
            }
        }

        static Vector3 CubeFacePoint(int face, float u, float v)
        {
            FaceBasis basis = GetFaceBasis(face);

            return Vector3Add(
                basis.normal,
                Vector3Add(
                    Vector3Scale(basis.axisA, u),
                    Vector3Scale(basis.axisB, v)
                )
            );
        }

        static Vector3 CubeToSphere(Vector3 p)
        {
            // Spherified cube. Better distribution than plain normalize.
            float x = p.x;
            float y = p.y;
            float z = p.z;

            float x2 = x * x;
            float y2 = y * y;
            float z2 = z * z;

            return Vector3{
                x * sqrtf(1.0f - y2 * 0.5f - z2 * 0.5f + y2 * z2 / 3.0f),
                y * sqrtf(1.0f - z2 * 0.5f - x2 * 0.5f + z2 * x2 / 3.0f),
                z * sqrtf(1.0f - x2 * 0.5f - y2 * 0.5f + x2 * y2 / 3.0f)
            };
        }

        static void AddFace(
            const GlobalObject& object,
            int face,
            int resolution,
            std::vector<VertexData>& vertices,
            std::vector<unsigned short>& indices
        )
        {
            const int startIndex = static_cast<int>(vertices.size());

            for (int y = 0; y <= resolution; ++y)
            {
                for (int x = 0; x <= resolution; ++x)
                {
                    float u = -1.0f + 2.0f * static_cast<float>(x) / static_cast<float>(resolution);
                    float v = -1.0f + 2.0f * static_cast<float>(y) / static_cast<float>(resolution);

                    Vector3 cubePoint = CubeFacePoint(face, u, v);
                    Vector3 direction = Vector3Normalize(CubeToSphere(cubePoint));

                    PlanetTerrainSample sample = SamplePlanetTerrain(object, direction);

                    VertexData vertex{};
                    vertex.position = Vector3Scale(direction, 1.0f + sample.height);
                    vertex.normal = EstimatePlanetNormal(object, direction);
                    vertex.color = sample.color;

                    vertices.push_back(vertex);
                }
            }

            for (int y = 0; y < resolution; ++y)
            {
                for (int x = 0; x < resolution; ++x)
                {
                    int i0 = startIndex + y * (resolution + 1) + x;
                    int i1 = i0 + 1;
                    int i2 = i0 + (resolution + 1);
                    int i3 = i2 + 1;

                    indices.push_back(static_cast<unsigned short>(i0));
                    indices.push_back(static_cast<unsigned short>(i1));
                    indices.push_back(static_cast<unsigned short>(i2));

                    indices.push_back(static_cast<unsigned short>(i1));
                    indices.push_back(static_cast<unsigned short>(i3));
                    indices.push_back(static_cast<unsigned short>(i2));
                }
            }
        }
    }

    Model GenerateLowDetailPlanetModel(const GlobalObject& object, int faceResolution)
    {
        std::vector<VertexData> vertices;
        std::vector<unsigned short> indices;

        const std::filesystem::path cachePath = GetPlanetMeshCachePath(
            object,
            faceResolution
        );

        const bool loadedFromCache = LoadPlanetMeshCache(
            cachePath,
            vertices,
            indices
        );

        if (!loadedFromCache)
        {
            vertices.reserve(6 * (faceResolution + 1) * (faceResolution + 1));
            indices.reserve(6 * faceResolution * faceResolution * 6);

            for (int face = 0; face < 6; ++face)
            {
                AddFace(object, face, faceResolution, vertices, indices);
            }

            SavePlanetMeshCache(cachePath, vertices, indices);
        }

        Mesh mesh{};
        mesh.vertexCount = static_cast<int>(vertices.size());
        mesh.triangleCount = static_cast<int>(indices.size() / 3);

        mesh.vertices = static_cast<float*>(
            MemAlloc(mesh.vertexCount * 3 * sizeof(float))
        );

        mesh.normals = static_cast<float*>(
            MemAlloc(mesh.vertexCount * 3 * sizeof(float))
        );

        mesh.colors = static_cast<unsigned char*>(
            MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char))
        );

        mesh.indices = static_cast<unsigned short*>(
            MemAlloc(indices.size() * sizeof(unsigned short))
        );

        for (int i = 0; i < mesh.vertexCount; ++i)
        {
            const VertexData& vertex = vertices[i];

            mesh.vertices[i * 3 + 0] = vertex.position.x;
            mesh.vertices[i * 3 + 1] = vertex.position.y;
            mesh.vertices[i * 3 + 2] = vertex.position.z;

            mesh.normals[i * 3 + 0] = vertex.normal.x;
            mesh.normals[i * 3 + 1] = vertex.normal.y;
            mesh.normals[i * 3 + 2] = vertex.normal.z;

            mesh.colors[i * 4 + 0] = vertex.color.r;
            mesh.colors[i * 4 + 1] = vertex.color.g;
            mesh.colors[i * 4 + 2] = vertex.color.b;
            mesh.colors[i * 4 + 3] = vertex.color.a;
        }

        for (int i = 0; i < static_cast<int>(indices.size()); ++i)
        {
            mesh.indices[i] = indices[i];
        }

        UploadMesh(&mesh, false);

        return LoadModelFromMesh(mesh);
    }
}