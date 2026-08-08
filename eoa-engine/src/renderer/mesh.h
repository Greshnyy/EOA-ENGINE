#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "renderer/vertex.h"

namespace eoa {

// GPU-буферы одного меша. Данные загружаются один раз через staging buffer
// (CPU-видимый -> DEVICE_LOCAL), дальше живут только на GPU — так быстрее
// рендерить, чем держать вершины в HOST_VISIBLE памяти каждый кадр.
class Mesh {
public:
    Mesh(VkPhysicalDevice physical, VkDevice device, VkCommandPool pool, VkQueue queue,
         const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void Bind(VkCommandBuffer cmd) const;
    uint32_t IndexCount() const { return indexCount_; }

private:
    VkDevice device_;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory_ = VK_NULL_HANDLE;
    uint32_t indexCount_ = 0;
};

} // namespace eoa
