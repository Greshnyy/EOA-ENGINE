#include "renderer/mesh_component.h"
#include "core/actor.h"
#include <glm/gtc/type_ptr.hpp>

namespace eoa {

MeshComponent::MeshComponent(VkPhysicalDevice physical, VkDevice device,
                             VkCommandPool pool, VkQueue queue,
                             const std::string& name)
    : physical_(physical), device_(device), pool_(pool), queue_(queue) {
    SetName(name);
}

void MeshComponent::BindDraw(VkCommandBuffer cmd, VkPipelineLayout layout) const {
    if (!mesh_ || !owner_) return;

    mesh_->Bind(cmd);

    auto* transform = owner_->GetComponent<TransformComponent>();
    glm::mat4 model = transform ? transform->GetModelMatrix() : glm::mat4(1.0f);

    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(glm::mat4), glm::value_ptr(model));

    vkCmdDrawIndexed(cmd, mesh_->IndexCount(), 1, 0, 0, 0);
}

} // namespace eoa
