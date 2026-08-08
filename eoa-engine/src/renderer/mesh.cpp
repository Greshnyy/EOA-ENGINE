#include "renderer/mesh.h"
#include "renderer/buffer_utils.h"
#include "log.h"
#include <cstring>

namespace eoa {

namespace {

// Общий паттерн для vertex и index буферов: staging (HOST_VISIBLE) -> DEVICE_LOCAL.
template <typename T>
void UploadDeviceLocal(VkPhysicalDevice physical, VkDevice device, VkCommandPool pool,
                       VkQueue queue, const std::vector<T>& data, VkBufferUsageFlags usage,
                       VkBuffer& outBuffer, VkDeviceMemory& outMemory) {
    VkDeviceSize size = sizeof(T) * data.size();

    VkBuffer staging;
    VkDeviceMemory stagingMemory;
    CreateBuffer(physical, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMemory);

    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    std::memcpy(mapped, data.data(), static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMemory);

    CreateBuffer(physical, device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

    CopyBuffer(device, pool, queue, staging, outBuffer, size);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

} // namespace

Mesh::Mesh(VkPhysicalDevice physical, VkDevice device, VkCommandPool pool, VkQueue queue,
           const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
    : device_(device), indexCount_(static_cast<uint32_t>(indices.size())) {
    UploadDeviceLocal(physical, device, pool, queue, vertices,
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBuffer_, vertexMemory_);
    UploadDeviceLocal(physical, device, pool, queue, indices,
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBuffer_, indexMemory_);

    EOA_LOG("Mesh uploaded: %zu vertices, %zu indices", vertices.size(), indices.size());
}

void Mesh::Bind(VkCommandBuffer cmd) const {
    VkBuffer buffers[] = {vertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
    vkCmdBindIndexBuffer(cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
}

Mesh::~Mesh() {
    vkDestroyBuffer(device_, indexBuffer_, nullptr);
    vkFreeMemory(device_, indexMemory_, nullptr);
    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vkFreeMemory(device_, vertexMemory_, nullptr);
}

} // namespace eoa
