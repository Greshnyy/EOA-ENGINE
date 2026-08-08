#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>
#include "renderer/mesh.h"
#include "renderer/texture.h"

namespace eoa {

struct Material {
    std::string name;
    std::unique_ptr<Texture> texture;
    glm::vec3 baseColor = glm::vec3(0.8f, 0.7f, 0.5f);
    float roughness = 1.0f;
    float metallic = 0.0f;
    // Descriptor set (set=1, binding=0: combined image sampler) под texture этого
    // материала. Владеет Renderer (allocated из общего descriptorPool_), сюда только
    // ссылка для удобства биндинга при отрисовке — Material сам его не создаёт/не удаляет.
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct GameObject {
    std::string name;
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    std::unique_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
    bool visible = true;

    glm::mat4 ModelMatrix() const {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position);
        m = m * glm::mat4_cast(rotation);
        m = glm::scale(m, scale);
        return m;
    }
};

class Scene {
public:
    std::vector<std::unique_ptr<GameObject>> objects;
    std::vector<std::shared_ptr<Material>> materials;

    GameObject* AddObject(const std::string& name) {
        auto obj = std::make_unique<GameObject>();
        obj->name = name;
        GameObject* ptr = obj.get();
        objects.push_back(std::move(obj));
        return ptr;
    }

    std::shared_ptr<Material> AddMaterial(const std::string& name) {
        auto mat = std::make_shared<Material>();
        mat->name = name;
        materials.push_back(mat);
        return mat;
    }

    void Clear() {
        objects.clear();
        materials.clear();
    }
};

} // namespace eoa