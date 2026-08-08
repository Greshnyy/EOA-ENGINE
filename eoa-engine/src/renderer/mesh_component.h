#pragma once
#include "core/component.h"
#include "core/transform_component.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace eoa {

// Материал, перенесённый из старого scene.h.
// Теперь живёт внутри MeshComponent, а не как отдельная сущность в сцене.
struct MaterialData {
    std::string name;
    std::unique_ptr<Texture> texture;
    glm::vec3 baseColor = glm::vec3(0.8f, 0.7f, 0.5f);
    float roughness = 1.0f;
    float metallic = 0.0f;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

class MeshComponent : public Component {
public:
    const char* ClassName() const override { return "MeshComponent"; }

    MeshComponent(VkPhysicalDevice physical, VkDevice device,
                  VkCommandPool pool, VkQueue queue,
                  const std::string& name = "Mesh");
    ~MeshComponent() override = default;

    void SetMesh(std::unique_ptr<Mesh> mesh) { mesh_ = std::move(mesh); }
    Mesh* GetMesh() const { return mesh_.get(); }
    bool HasMesh() const { return mesh_ != nullptr; }

    void SetMaterial(std::shared_ptr<MaterialData> mat) { material_ = mat; }
    std::shared_ptr<MaterialData> GetMaterial() const { return material_; }

    // Вызывается рендерером: биндит вершинный/индексный буфер, пушит model-матрицу
    void BindDraw(VkCommandBuffer cmd, VkPipelineLayout layout) const;

private:
    VkPhysicalDevice physical_;
    VkDevice device_;
    VkCommandPool pool_;
    VkQueue queue_;
    std::unique_ptr<Mesh> mesh_;
    std::shared_ptr<MaterialData> material_;
};

} // namespace eoa
