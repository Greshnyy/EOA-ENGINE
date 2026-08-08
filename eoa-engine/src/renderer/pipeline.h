#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace eoa {

// Pipeline с реальным vertex input, depth testing, push constant (per-object
// model-матрица) и ДВУМЯ descriptor set layout'ами: set=0 — UBO камеры/света
// (общий на кадр), set=1 — texture материала (свой на каждый Material, не
// на кадр). Разделение важно: если бы текстура объекта была в том же наборе,
// что и UBO, пришлось бы плодить по набору на (кадр × материал); так —
// набор на кадр под камеру плюс один набор на материал, который живёт
// сколько живёт сам Material, не пересоздаётся каждый кадр.
class Pipeline {
public:
    Pipeline(VkDevice device, VkRenderPass renderPass,
              VkDescriptorSetLayout cameraSetLayout, VkDescriptorSetLayout materialSetLayout,
              const std::string& vertSpvPath, const std::string& fragSpvPath,
              uint32_t pushConstantSize = 64,
              VkShaderStageFlags pushStage = VK_SHADER_STAGE_VERTEX_BIT,
              VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
              VkBool32 depthTest = VK_TRUE,
              VkBool32 depthWrite = VK_TRUE);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    VkPipeline Handle() const { return pipeline_; }
    VkPipelineLayout Layout() const { return layout_; }

private:
    VkDevice device_;
    VkPipelineLayout layout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
};

} // namespace eoa
