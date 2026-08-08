#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

namespace eoa {

// GBuffer: 3 цветовых аттачмента (Albedo, Normal, ORM) + depth.
// Создаётся и управляется Renderer-ом.
class GBuffer {
public:
    static constexpr uint32_t kColorAttachmentCount = 3;

    GBuffer(VkPhysicalDevice physical, VkDevice device,
            uint32_t width, uint32_t height,
            VkFormat depthFormat);
    ~GBuffer();

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

    void Resize(uint32_t width, uint32_t height);

    VkRenderPass GetRenderPass() const { return renderPass_; }
    VkFramebuffer GetFramebuffer() const { return framebuffer_; }
    VkExtent2D GetExtent() const { return extent_; }

    // Для биндинга в Deferred Lighting pass
    VkImageView GetAlbedoView() const { return albedoView_; }
    VkImageView GetNormalView() const { return normalView_; }
    VkImageView GetORMView() const { return ormView_; }
    VkImageView GetDepthView() const { return depthView_; }

    // Sampler для всех GBuffer текстур
    VkSampler GetSampler() const { return sampler_; }

    // Форматы аттачментов
    static VkFormat AlbedoFormat();   // R8G8B8A8_SRGB
    static VkFormat NormalFormat();   // R16G16B16A16_SFLOAT
    static VkFormat ORMFormat();      // R8G8B8A8_UNORM

private:
    void CreateImages();
    void CreateRenderPass();
    void CreateFramebuffer();
    void DestroyImages();

    VkPhysicalDevice physical_;
    VkDevice device_;
    VkFormat depthFormat_;
    VkExtent2D extent_{};

    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

    // Color attachments
    VkImage albedoImage_ = VK_NULL_HANDLE;
    VkDeviceMemory albedoMemory_ = VK_NULL_HANDLE;
    VkImageView albedoView_ = VK_NULL_HANDLE;

    VkImage normalImage_ = VK_NULL_HANDLE;
    VkDeviceMemory normalMemory_ = VK_NULL_HANDLE;
    VkImageView normalView_ = VK_NULL_HANDLE;

    VkImage ormImage_ = VK_NULL_HANDLE;
    VkDeviceMemory ormMemory_ = VK_NULL_HANDLE;
    VkImageView ormView_ = VK_NULL_HANDLE;

    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
    VkImageView depthView_ = VK_NULL_HANDLE;

    VkSampler sampler_ = VK_NULL_HANDLE;
};

} // namespace eoa
