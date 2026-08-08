#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

namespace eoa {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    static VkVertexInputBindingDescription BindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 4> AttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attrs{};
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, normal);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, color);

        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[3].offset = offsetof(Vertex, uv);
        return attrs;
    }
};

// model больше не здесь — он per-object и идёт через push constant
// (см. pipeline.cpp), потому что здесь он был бы общим на весь кадр,
// а не на каждый объект.
struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 lightDir;   // xyz = direction to sun, w = intensity
    glm::vec4 lightColor; // rgb + unused
};

} // namespace eoa