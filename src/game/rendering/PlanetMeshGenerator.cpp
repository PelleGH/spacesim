#include "rendering/PlanetMeshGenerator.h"

#include "world/PlanetTerrainGenerator.h"

#include <raymath.h>
#include <rlgl.h>

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

        vertices.reserve(6 * (faceResolution + 1) * (faceResolution + 1));
        indices.reserve(6 * faceResolution * faceResolution * 6);

        for (int face = 0; face < 6; ++face)
        {
            AddFace(object, face, faceResolution, vertices, indices);
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