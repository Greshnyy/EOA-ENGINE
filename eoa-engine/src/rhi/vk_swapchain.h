#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace eoa {

// Swapchain + его image views. Пересоздаётся целиком при resize окна —
// это стандартный Vulkan-подход, не оптимизация на потом.
class VulkanSwapchain {
public:
    VulkanSwapchain(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface,
                     uint32_t graphicsFamily, uint32_t width, uint32_t height);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain&) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

    VkSwapchainKHR Handle() const { return swapchain_; }
    VkFormat ImageFormat() const { return imageFormat_; }
    VkExtent2D Extent() const { return extent_; }
    const std::vector<VkImageView>& ImageViews() const { return imageViews_; }
    uint32_t ImageCount() const { return static_cast<uint32_t>(images_.size()); }

private:
    VkDevice device_ = VK_NULL_HANDLE; // не владеем, храним для очистки
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_;
    VkExtent2D extent_;

    void CreateImageViews();
};

} // namespace eoa
