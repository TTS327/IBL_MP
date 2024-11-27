#include "ModelLoader.h"

#include <filesystem>

namespace FEFE 
{

using namespace DirectX::SimpleMath;

void ModelLoader::Load(std::string basePath, std::string filename) 
{
    this->basePath = basePath;
     
    Assimp::Importer importer;

    const aiScene *pScene = importer.ReadFile(
        this->basePath + filename,
        aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

    if (!pScene) {
        std::cout << "Failed to read file: " << this->basePath + filename
                  << std::endl;
    } else {
        Matrix tr; // Initial transformation
        ProcessNode(pScene->mRootNode, pScene, tr);
    }
}

void ModelLoader::ProcessNode(aiNode *node, const aiScene *scene, Matrix tr) 
{

    Matrix m;
    ai_real *temp = &node->mTransformation.a1;
    float *mTemp = &m._11;
    for (int t = 0; t < 16; t++) 
    {
        mTemp[t] = float(temp[t]);
    }
    m = m.Transpose() * tr;

    for (UINT i = 0; i < node->mNumMeshes; i++) 
    {

        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        auto newMesh = this->ProcessMesh(mesh, scene);

        for (auto &v : newMesh.vertices) 
        {
            v.position = DirectX::SimpleMath::Vector3::Transform(v.position, m);
        }

        meshes.push_back(newMesh);
    }

    for (UINT i = 0; i < node->mNumChildren; i++) 
    {
        this->ProcessNode(node->mChildren[i], scene, m);
    }
}

MeshData ModelLoader::ProcessMesh(aiMesh *mesh, const aiScene *scene)
{
    // cpu에서 정의
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

   
    for (UINT i = 0; i < mesh->mNumVertices; i++) 
    {
        Vertex vertex;

        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;
        vertex.normal.Normalize();

        if (mesh->mTextureCoords[0]) 
        {
            vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
            vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(vertex);
    }

    for (UINT i = 0; i < mesh->mNumFaces; i++) 
    {
        aiFace face = mesh->mFaces[i];
        for (UINT j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    MeshData newMesh;
    newMesh.vertices = vertices;
    newMesh.indices = indices;

    // http://assimp.sourceforge.net/lib_html/materials.html
    if (mesh->mMaterialIndex >= 0) 
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString filepath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &filepath);

            std::string fullPath =
                this->basePath +
                std::string(std::filesystem::path(filepath.C_Str())
                                .filename()
                                .string());

            newMesh.textureFilename = fullPath;
        }
    }

    return newMesh;
}

} // namespace FEFE