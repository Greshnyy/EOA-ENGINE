#pragma once
#include <vulkan/vulkan.h>
#include <optional>

namespace eoa {

// Выбирает физический GPU и создаёт логическое устройство + очередь.
// Пока только graphics+present очередь — этого достаточно для первого
// milestone (окно + отрисовка clear color). Compute-очередь для тумана/
// аномалий добавится отдельным шагом позже (см. план движка).
class VulkanDevice {
public:
    VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    VkPhysicalDevice Physical() const { return physical_; }
    VkDevice Logical() const { return logical_; }
    VkQueue GraphicsQueue() const { return graphicsQueue_; }
    uint32_t GraphicsQueueFamily() const { return graphicsFamily_; }

private:
    VkPhysicalDevice physical_ = VK_NULL_HANDLE;
    VkDevice logical_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsFamily_ = 0;

    void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    void CreateLogicalDevice();
};

} // namespace eoa
