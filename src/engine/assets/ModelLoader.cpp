#include "assets/ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace SpaceSim
{
    ModelData ModelLoader::load(
        const std::filesystem::path& path)
    {
        Assimp::Importer importer;

        const unsigned int flags =
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_ImproveCacheLocality |
            aiProcess_SortByPType |
            aiProcess_PreTransformVertices;

        const aiScene* scene =
            importer.ReadFile(
                path.string(),
                flags);

        if (!scene ||
            !scene->mRootNode ||
            scene->mNumMeshes == 0)
        {
            throw std::runtime_error(
                "Failed to load model '" +
                path.string() +
                "': " +
                importer.GetErrorString());
        }

        ModelData result;

        glm::vec3 minimum(
            std::numeric_limits<float>::max());

        glm::vec3 maximum(
            std::numeric_limits<float>::lowest());

        result.meshes.reserve(
            scene->mNumMeshes);

        for (unsigned int meshIndex = 0;
             meshIndex < scene->mNumMeshes;
             ++meshIndex)
        {
            const aiMesh* sourceMesh =
                scene->mMeshes[meshIndex];

            if (!sourceMesh ||
                sourceMesh->mNumVertices == 0)
            {
                continue;
            }

            MeshData mesh;

            mesh.vertices.reserve(
                sourceMesh->mNumVertices);

            for (unsigned int vertexIndex = 0;
                 vertexIndex < sourceMesh->mNumVertices;
                 ++vertexIndex)
            {
                const aiVector3D& sourcePosition =
                    sourceMesh->mVertices[vertexIndex];

                MeshVertex vertex{};

                vertex.position =
                {
                    sourcePosition.x,
                    sourcePosition.y,
                    sourcePosition.z
                };

                if (sourceMesh->HasNormals())
                {
                    const aiVector3D& sourceNormal =
                        sourceMesh->mNormals[vertexIndex];

                    vertex.normal =
                    {
                        sourceNormal.x,
                        sourceNormal.y,
                        sourceNormal.z
                    };
                }
                else
                {
                    // GenSmoothNormals should normally prevent
                    // us from reaching this fallback.
                    vertex.normal =
                    {
                        0.0f,
                        1.0f,
                        0.0f
                    };
                }

                if (sourceMesh->HasTextureCoords(0))
                {
                    const aiVector3D& sourceUv =
                        sourceMesh->mTextureCoords[0][vertexIndex];

                    vertex.texCoord =
                    {
                        sourceUv.x,
                        sourceUv.y
                    };
                }
                else
                {
                    vertex.texCoord =
                    {
                        0.0f,
                        0.0f
                    };
                }

                minimum.x =
                    std::min(
                        minimum.x,
                        vertex.position.x);

                minimum.y =
                    std::min(
                        minimum.y,
                        vertex.position.y);

                minimum.z =
                    std::min(
                        minimum.z,
                        vertex.position.z);

                maximum.x =
                    std::max(
                        maximum.x,
                        vertex.position.x);

                maximum.y =
                    std::max(
                        maximum.y,
                        vertex.position.y);

                maximum.z =
                    std::max(
                        maximum.z,
                        vertex.position.z);

                mesh.vertices.push_back(
                    vertex);
            }

            for (unsigned int faceIndex = 0;
                 faceIndex < sourceMesh->mNumFaces;
                 ++faceIndex)
            {
                const aiFace& face =
                    sourceMesh->mFaces[faceIndex];

                for (unsigned int index = 0;
                     index < face.mNumIndices;
                     ++index)
                {
                    mesh.indices.push_back(
                        face.mIndices[index]);
                }
            }

            if (!mesh.indices.empty())
            {
                result.meshes.push_back(
                    std::move(mesh));
            }
        }

        if (result.meshes.empty())
        {
            throw std::runtime_error(
                "Model contained no renderable meshes: " +
                path.string());
        }

        result.boundsMin = minimum;
        result.boundsMax = maximum;

        return result;
    }
}