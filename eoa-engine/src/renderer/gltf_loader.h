#pragma once
#include <string>
#include <vector>
#include <memory>
#include "renderer/vertex.h"

namespace eoa {

struct GltfMaterial {
    std::string name;
    std::string albedoTexturePath;
    float metallicFactor = 0.0f;
    float roughnessFactor = 1.0f;
};

struct GltfNode {
    std::string name;
    int meshIndex = -1;
    std::vector<float> matrix; // 4x4 column-major, 16 floats
    std::vector<int> children;
};

struct GltfModel {
    std::vector<std::vector<Vertex>> meshVertices;
    std::vector<std::vector<uint16_t>> meshIndices;
    // Материал первого primitive каждого меша, -1 если материала нет.
    // Упрощение: если у меша несколько primitives с РАЗНЫМИ материалами,
    // они всё равно склеены в один vertex/index буфер (см. GltfLoader::Load) —
    // так что здесь физически может быть только один материал на меш.
    std::vector<int> meshMaterialIndex;
    std::vector<GltfMaterial> materials;
    std::vector<GltfNode> nodes;
    std::vector<int> rootNodes;
};

class GltfLoader {
public:
    static GltfModel Load(const std::string& path);
};

} // namespace eoa