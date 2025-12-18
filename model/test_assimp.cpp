#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

void TestAssimp()
{
    Assimp::Importer importer;
    
    // 尝试加载一个模型文件
    const aiScene* scene = importer.ReadFile("path/to/your/model.obj",
        aiProcess_Triangulate | 
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    
    std::cout << "Assimp loaded successfully!" << std::endl;
    std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Materials: " << scene->mNumMaterials << std::endl;
}