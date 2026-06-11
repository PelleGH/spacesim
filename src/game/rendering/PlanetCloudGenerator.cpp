#include "rendering/PlanetCloudGenerator.h"

#include <raymath.h>
#include <rlgl.h>

#include <cmath>
#include <vector>

namespace SpaceSim
{
    namespace
    {
        struct VertexData
        {
            Vector3 position{};
            Vector3 normal{};
            Color color = BLANK;
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
            case 0:
                return FaceBasis{ Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 }, Vector3{ 0, 1, 0 } };
            case 1:
                return FaceBasis{ Vector3{ -1, 0, 0 }, Vector3{ 0, 0, 1 }, Vector3{ 0, 1, 0 } };
            case 2:
                return FaceBasis{ Vector3{ 0, 1, 0 }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, -1 } };
            case 3:
                return FaceBasis{ Vector3{ 0, -1, 0 }, Vector3{ 1, 0, 0 }, Vector3{ 0, 0, 1 } };
            case 4:
                return FaceBasis{ Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, Vector3{ 0, 1, 0 } };
            case 5:
                return FaceBasis{ Vector3{ 0, 0, -1 }, Vector3{ -1, 0, 0 }, Vector3{ 0, 1, 0 } };
            default:
                return FaceBasis{ Vector3{ 0, 0, 1 }, Vector3{ 1, 0, 0 }, Vector3{ 0, 1, 0 } };
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

        static float HashNoise(float x, float y, float z, unsigned int seed)
        {
            float value = sinf(
                x * 12.9898f +
                y * 78.233f +
                z * 37.719f +
                static_cast<float>(seed) * 0.013f
            ) * 43758.5453f;

            return value - floorf(value);
        }

        static float SmoothNoise3D(Vector3 p, unsigned int seed)
        {
            Vector3 base{
                floorf(p.x),
                floorf(p.y),
                floorf(p.z)
            };

            Vector3 f{
                p.x - base.x,
                p.y - base.y,
                p.z - base.z
            };

            f.x = f.x * f.x * (3.0f - 2.0f * f.x);
            f.y = f.y * f.y * (3.0f - 2.0f * f.y);
            f.z = f.z * f.z * (3.0f - 2.0f * f.z);

            auto sample = [&](int dx, int dy, int dz)
            {
                return HashNoise(base.x + dx, base.y + dy, base.z + dz, seed);
            };

            float x00 = sample(0, 0, 0) * (1.0f - f.x) + sample(1, 0, 0) * f.x;
            float x10 = sample(0, 1, 0) * (1.0f - f.x) + sample(1, 1, 0) * f.x;
            float x01 = sample(0, 0, 1) * (1.0f - f.x) + sample(1, 0, 1) * f.x;
            float x11 = sample(0, 1, 1) * (1.0f - f.x) + sample(1, 1, 1) * f.x;

            float y0 = x00 * (1.0f - f.y) + x10 * f.y;
            float y1 = x01 * (1.0f - f.y) + x11 * f.y;

            return y0 * (1.0f - f.z) + y1 * f.z;
        }

        static float FractalNoise3D(Vector3 direction, float frequency, unsigned int seed)
        {
            float total = 0.0f;
            float amplitude = 0.5f;
            float maxValue = 0.0f;

            Vector3 p = Vector3Scale(direction, frequency);

            for (int i = 0; i < 5; ++i)
            {
                total += SmoothNoise3D(p, seed + static_cast<unsigned int>(i * 97)) * amplitude;
                maxValue += amplitude;

                p = Vector3Scale(p, 2.03f);
                amplitude *= 0.5f;
            }

            return total / maxValue;
        }

        static float SmoothStep(float value)
        {
            value = Clamp(value, 0.0f, 1.0f);
            return value * value * (3.0f - 2.0f * value);
        }

        static Color CloudColorFromDirection(const GlobalObject& object, Vector3 direction)
        {
            (void)object;
            (void)direction;

            // The cloud shader will generate the actual cloud shape per pixel.
            // The mesh only needs to provide a white shell.
            return Color{ 255, 255, 255, 255 };
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

                    VertexData vertex{};
                    vertex.position = direction;
                    vertex.normal = direction;
                    vertex.color = CloudColorFromDirection(object, direction);

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

    Model GenerateCloudLayerModel(const GlobalObject& object, int faceResolution)
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
            const VertexData& vertex = vertices[static_cast<size_t>(i)];

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
            mesh.indices[i] = indices[static_cast<size_t>(i)];
        }

        UploadMesh(&mesh, false);

        return LoadModelFromMesh(mesh);
    }
}