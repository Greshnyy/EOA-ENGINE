#pragma once
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <vector>

namespace eoa {

class GBuffer;

// Deferred Lighting: fullscreen pass, читает GBuffer, вычисляет PBR-освещение
// для всех источников света, выводит результат в swapchain.
class DeferredLighting {
public:
    DeferredLighting(VkPhysicalDevice physical, VkDevice device,
                     VkRenderPass outputRenderPass,
                     uint32_t subpassIndex);
    ~DeferredLighting();

    DeferredLighting(const DeferredLighting&) = delete;
    DeferredLighting& operator=(const DeferredLighting&) = delete;

    // Создаёт pipeline с указанными дескриптор-лейаутами
    void CreatePipeline(VkDescriptorSetLayout cameraSetLayout,
                        VkDescriptorSetLayout materialSetLayout,
                        const std::string& vertSpvPath,
                        const std::string& fragSpvPath);

    VkPipeline GetPipeline() const { return pipeline_; }
    VkPipelineLayout GetLayout() const { return layout_; }
    VkDescriptorSet GetDescriptorSet() const { return descriptorSet_; }

    // Биндит GBuffer текстуры в descriptor set
    void UpdateDescriptorSet(const GBuffer& gbuffer);

private:
    VkPhysicalDevice physical_;
    VkDevice device_;
    VkRenderPass outputRenderPass_;
    uint32_t subpassIndex_;

    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
};

} // namespace eoa
