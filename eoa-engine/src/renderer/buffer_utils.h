#pragma once
#include <vulkan/vulkan.h>

namespace eoa {

// Находит индекс типа памяти GPU с нужными флагами свойств.
uint32_t FindMemoryType(VkPhysicalDevice physical, uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

// Создаёт VkBuffer + выделяет и биндит под него память.
void CreateBuffer(VkPhysicalDevice physical, VkDevice device, VkDeviceSize size,
                   VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                   VkBuffer& outBuffer, VkDeviceMemory& outMemory);

// Копирует один буфер в другой через одноразовый командный буфер.
void CopyBuffer(VkDevice device, VkCommandPool pool, VkQueue queue,
                 VkBuffer src, VkBuffer dst, VkDeviceSize size);

// Создаёт VkImage + выделяет и биндит память.
void CreateImage(VkPhysicalDevice physical, VkDevice device,
                 uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties,
                 VkImage& outImage, VkDeviceMemory& outMemory);

// Создаёт VkImageView для заданного image.
VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format,
                              VkImageAspectFlags aspectFlags);

} // namespace eoa
