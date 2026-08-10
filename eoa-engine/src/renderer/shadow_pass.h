#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include "renderer/pipeline.h"

namespace eoa {

// Shadow mapping pass: renders depth from light's view for directional light
class ShadowPass {
public:
    static constexpr uint32_t kShadowMapSize = 2048;

    ShadowPass(VkPhysicalDevice physical, VkDevice device, uint32_t graphicsFamily);
    ~ShadowPass();

    ShadowPass(const ShadowPass&) = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;

    void CreatePipeline(VkDescriptorSetLayout cameraSetLayout);
    void Resize(uint32_t width, uint32_t height);

    VkRenderPass GetRenderPass() const { return renderPass_; }
    VkFramebuffer GetFramebuffer() const { return framebuffer_; }
    VkPipeline GetPipeline() const { return pipeline_ ? pipeline_->Handle() : VK_NULL_HANDLE; }
    VkPipelineLayout GetLayout() const { return pipeline_ ? pipeline_->Layout() : VK_NULL_HANDLE; }
    VkImageView GetDepthImageView() const { return depthImageView_; }
    VkSampler GetDepthSampler() const { return depthSampler_; }
    VkExtent2D GetExtent() const { return extent_; }

    glm::mat4 GetLightViewProj() const { return lightViewProj_; }
    void SetLightViewProj(const glm::mat4& vp) { lightViewProj_ = vp; }

private:
    VkPhysicalDevice physical_;
    VkDevice device_;
    uint32_t graphicsFamily_;

    VkExtent2D extent_{kShadowMapSize, kShadowMapSize};
    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    VkSampler depthSampler_ = VK_NULL_HANDLE;

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;

    std::unique_ptr<Pipeline> pipeline_;
    glm::mat4 lightViewProj_ = glm::mat4(1.0f);

    void CreateImages();
    void CreateRenderPass();
    void CreateFramebuffer();
    void CreateCommandPool();
    void DestroyShadowResources();
};

} // namespace eoa
