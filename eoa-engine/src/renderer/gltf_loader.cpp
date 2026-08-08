#define TINYGLTF_IMPLEMENTATION
// НЕ определяем TINYGLTF_NO_STB_IMAGE: без него tinygltf сам решает, что делать
// с картинками, но без замены на свой image-loader callback это оставляет
// LoadImageData == nullptr, и парсинг ЛЮБОГО glTF с внешней текстурой падает
// с пустым err ("No LoadImageData callback specified" даже не долетает —
// проверено на реальном тестовом ассете). Раз stb_image уже слинкован
// (см. vendor/stb_image_impl.cpp), пусть tinygltf использует его штатно —
// декодированные пиксели из tinygltf нам не нужны (мы грузим текстуру
// заново через свой Texture/stb_image по пути из URI), но раз декодирование
// всё равно бесплатно идёт как часть парсинга — не боремся с этим отдельно.
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include "renderer/gltf_loader.h"
#include "log.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <limits>

namespace eoa {

namespace {

std::vector<float> GetAccessorData(const tinygltf::Model& model, int accessorIndex) {
    if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size())) {
        return {};
    }
    const auto& acc = model.accessors[accessorIndex];
    if (acc.bufferView < 0 || acc.bufferView >= static_cast<int>(model.bufferViews.size())) {
        return {};
    }
    const auto& bv = model.bufferViews[acc.bufferView];
    if (bv.buffer < 0 || bv.buffer >= static_cast<int>(model.buffers.size())) {
        return {};
    }
    const auto& buf = model.buffers[bv.buffer];

    size_t count = acc.count;
    size_t componentCount = tinygltf::GetNumComponentsInType(acc.type);
    size_t componentSize = tinygltf::GetComponentSizeInBytes(acc.componentType);
    size_t stride = bv.byteStride ? bv.byteStride : componentCount * componentSize;

    std::vector<float> result(count * componentCount);
    const uint8_t* src = buf.data.data() + bv.byteOffset + acc.byteOffset;

    for (size_t i = 0; i < count; ++i) {
        const uint8_t* elem = src + i * stride;
        for (size_t c = 0; c < componentCount; ++c) {
            float value = 0.0f;
            switch (acc.componentType) {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                std::memcpy(&value, elem + c * sizeof(float), sizeof(float));
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                value = static_cast<float>(elem[c]);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                value = static_cast<float>(reinterpret_cast<const uint16_t*>(elem)[c]);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                value = static_cast<float>(reinterpret_cast<const uint32_t*>(elem)[c]);
                break;
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                value = static_cast<float>(reinterpret_cast<const int8_t*>(elem)[c]);
                break;
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                value = static_cast<float>(reinterpret_cast<const int16_t*>(elem)[c]);
                break;
            default:
                break;
            }
            result[i * componentCount + c] = value;
        }
    }
    return result;
}

std::vector<uint16_t> GetIndicesData(const tinygltf::Model& model, int accessorIndex) {
    if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size())) {
        return {};
    }
    auto raw = GetAccessorData(model, accessorIndex);
    std::vector<uint16_t> indices(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        indices[i] = static_cast<uint16_t>(raw[i]);
    }
    return indices;
}

} // namespace

GltfModel GltfLoader::Load(const std::string& path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool ok = false;
    std::string ext = path.substr(path.find_last_of('.') + 1);
    if (ext == "glb") {
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    } else {
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    }

    if (!warn.empty()) EOA_LOG("glTF warning: %s", warn.c_str());
    if (!err.empty()) EOA_LOG("glTF error: %s", err.c_str());
    if (!ok) {
        EOA_WARN("Failed to load glTF: %s", path.c_str());
        return {};
    }

    GltfModel result;

    result.meshVertices.resize(model.meshes.size());
    result.meshIndices.resize(model.meshes.size());
    result.meshMaterialIndex.assign(model.meshes.size(), -1);

    for (size_t mi = 0; mi < model.meshes.size(); ++mi) {
        const auto& mesh = model.meshes[mi];
        std::vector<Vertex>& allVerts = result.meshVertices[mi];
        std::vector<uint16_t>& allIndices = result.meshIndices[mi];

        if (!mesh.primitives.empty()) {
            result.meshMaterialIndex[mi] = mesh.primitives[0].material;
        }

        for (size_t pi = 0; pi < mesh.primitives.size(); ++pi) {
            const auto& prim = mesh.primitives[pi];

            auto posData = GetAccessorData(model, prim.attributes.count("POSITION") ?
                prim.attributes.at("POSITION") : -1);
            auto normalData = GetAccessorData(model, prim.attributes.count("NORMAL") ?
                prim.attributes.at("NORMAL") : -1);
            auto uvData = GetAccessorData(model, prim.attributes.count("TEXCOORD_0") ?
                prim.attributes.at("TEXCOORD_0") : -1);

            auto idxData = GetIndicesData(model, prim.indices);

            size_t vertexCount = posData.size() / 3;
            size_t hasNormals = normalData.size() / 3 >= vertexCount;
            size_t hasUVs = uvData.size() / 2 >= vertexCount;

            uint16_t baseIndex = static_cast<uint16_t>(allVerts.size());

            for (size_t v = 0; v < vertexCount; ++v) {
                Vertex vert;
                vert.pos = glm::vec3(posData[v * 3], posData[v * 3 + 1], posData[v * 3 + 2]);
                vert.normal = hasNormals ?
                    glm::vec3(normalData[v * 3], normalData[v * 3 + 1], normalData[v * 3 + 2]) :
                    glm::vec3(0.0f, 0.0f, 1.0f);
                vert.uv = hasUVs ?
                    glm::vec2(uvData[v * 2], uvData[v * 2 + 1]) :
                    glm::vec2(0.0f);
                vert.color = glm::vec3(0.8f, 0.7f, 0.5f);
                allVerts.push_back(vert);
            }

            for (size_t i = 0; i < idxData.size(); ++i) {
                allIndices.push_back(baseIndex + idxData[i]);
            }
        }
    }

    for (const auto& mat : model.materials) {
        GltfMaterial gm;
        gm.name = mat.name;
        gm.metallicFactor = static_cast<float>(mat.pbrMetallicRoughness.metallicFactor);
        gm.roughnessFactor = static_cast<float>(mat.pbrMetallicRoughness.roughnessFactor);

        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0) {
            int texIdx = mat.pbrMetallicRoughness.baseColorTexture.index;
            if (texIdx < static_cast<int>(model.textures.size())) {
                int imgIdx = model.textures[texIdx].source;
                if (imgIdx >= 0 && imgIdx < static_cast<int>(model.images.size())) {
                    const std::string& uri = model.images[imgIdx].uri;
                    if (uri.rfind("data:", 0) == 0) {
                        // Embedded base64-текстуры пока не поддержаны — движок грузит
                        // текстуры через stb_image с диска, не из декодированной памяти.
                        // Для внешних .png/.jpg рядом с .gltf всё работает.
                        EOA_WARN("glTF material '%s': embedded (data:) текстура не "
                                 "поддерживается, только внешние файлы", gm.name.c_str());
                    } else if (!uri.empty()) {
                        // uri в glTF — относительно директории самого .gltf файла,
                        // а не рабочей директории движка, так что резолвим здесь.
                        size_t slash = path.find_last_of("/\\");
                        std::string baseDir = (slash == std::string::npos) ? "" :
                                                path.substr(0, slash + 1);
                        gm.albedoTexturePath = baseDir + uri;
                    }
                }
            }
        }
        result.materials.push_back(gm);
    }

    std::vector<std::vector<float>> nodeMatrices(model.nodes.size());
    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        const auto& node = model.nodes[ni];
        std::vector<float> matrix;
        matrix.reserve(16);
        if (node.matrix.size() == 16) {
            for (const auto& v : node.matrix) {
                matrix.push_back(static_cast<float>(v));
            }
        } else {
            glm::mat4 local = glm::mat4(1.0f);
            if (node.translation.size() == 3) {
                local = glm::translate(local, glm::vec3(
                    static_cast<float>(node.translation[0]),
                    static_cast<float>(node.translation[1]),
                    static_cast<float>(node.translation[2])));
            }
            if (node.rotation.size() == 4) {
                glm::quat q(
                    static_cast<float>(node.rotation[3]),
                    static_cast<float>(node.rotation[0]),
                    static_cast<float>(node.rotation[1]),
                    static_cast<float>(node.rotation[2]));
                local = local * glm::mat4_cast(q);
            }
            if (node.scale.size() == 3) {
                local = glm::scale(local, glm::vec3(
                    static_cast<float>(node.scale[0]),
                    static_cast<float>(node.scale[1]),
                    static_cast<float>(node.scale[2])));
            }
            const float* p = glm::value_ptr(local);
            matrix.assign(p, p + 16);
        }
        if (matrix.size() < 16) {
            matrix.resize(16, 0.0f);
            matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
        }
        nodeMatrices[ni] = matrix;
    }

    for (size_t ni = 0; ni < model.nodes.size(); ++ni) {
        GltfNode gn;
        gn.name = model.nodes[ni].name;
        gn.meshIndex = model.nodes[ni].mesh;
        gn.matrix = nodeMatrices[ni];
        gn.children = model.nodes[ni].children;
        result.nodes.push_back(gn);
    }

    if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size())) {
        const auto& scene = model.scenes[model.defaultScene];
        result.rootNodes = scene.nodes;
    } else if (!model.scenes.empty()) {
        result.rootNodes = model.scenes[0].nodes;
    } else if (!model.nodes.empty()) {
        result.rootNodes.push_back(0);
    }

    EOA_LOG("glTF loaded: %s — %zu meshes, %zu materials, %zu nodes",
            path.c_str(), result.meshVertices.size(), result.materials.size(),
            result.nodes.size());

    return result;
}

} // namespace eoa