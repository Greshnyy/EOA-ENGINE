#pragma once
#include <vulkan/vulkan.h>
#include <string>

namespace eoa {

// Загружает изображение с диска (через stb_image — PNG/JPG/BMP/TGA), заливает
// в DEVICE_LOCAL VkImage через staging buffer, создаёт view + sampler.
// Без мип-уровней и компрессии пока — это отдельный шаг оптимизации на потом.
class Texture {
public:
    Texture(VkPhysicalDevice physical, VkDevice device, VkCommandPool pool, VkQueue queue,
             const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    VkImageView View() const { return view_; }
    VkSampler Sampler() const { return sampler_; }

private:
    VkDevice device_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;

    void TransitionLayout(VkCommandPool pool, VkQueue queue, VkImageLayout oldLayout,
                            VkImageLayout newLayout);
    void CopyBufferToImage(VkCommandPool pool, VkQueue queue, VkBuffer buffer,
                             uint32_t width, uint32_t height);
};

} // namespace eoa
