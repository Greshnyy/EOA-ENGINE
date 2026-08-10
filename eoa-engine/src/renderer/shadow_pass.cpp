#include "renderer/shadow_pass.h"
#include "renderer/pipeline.h"
#include "renderer/buffer_utils.h"
#include "log.h"
#include <vulkan/vulkan.h>

namespace eoa {

ShadowPass::ShadowPass(VkPhysicalDevice physical, VkDevice device, uint32_t graphicsFamily)
    : physical_(physical), device_(device), graphicsFamily_(graphicsFamily) {
    CreateImages();
    CreateRenderPass();
    CreateFramebuffer();
    CreateCommandPool();
}

ShadowPass::~ShadowPass() {
    DestroyShadowResources();
}

void ShadowPass::CreateImages() {
    // Depth image for shadow map
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {extent_.width, extent_.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    EOA_CHECK_VK(vkCreateImage(device_, &imageInfo, nullptr, &depthImage_));

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device_, depthImage_, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physical_, memReq.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    EOA_CHECK_VK(vkAllocateMemory(device_, &allocInfo, nullptr, &depthImageMemory_));
    EOA_CHECK_VK(vkBindImageMemory(device_, depthImage_, depthImageMemory_, 0));

    // Image view for depth
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    EOA_CHECK_VK(vkCreateImageView(device_, &viewInfo, nullptr, &depthImageView_));

    // Sampler for shadow map (compare enabled for PCF)
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    EOA_CHECK_VK(vkCreateSampler(device_, &samplerInfo, nullptr, &depthSampler_));

    EOA_LOG("Shadow map created: %ux%u", extent_.width, extent_.height);
}

void ShadowPass::CreateRenderPass() {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &depthAttachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    EOA_CHECK_VK(vkCreateRenderPass(device_, &info, nullptr, &renderPass_));
}

void ShadowPass::CreateFramebuffer() {
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass_;
    info.attachmentCount = 1;
    info.pAttachments = &depthImageView_;
    info.width = extent_.width;
    info.height = extent_.height;
    info.layers = 1;

    EOA_CHECK_VK(vkCreateFramebuffer(device_, &info, nullptr, &framebuffer_));
}

void ShadowPass::CreateCommandPool() {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = graphicsFamily_;
    EOA_CHECK_VK(vkCreateCommandPool(device_, &info, nullptr, &commandPool_));
}

void ShadowPass::CreatePipeline(VkDescriptorSetLayout cameraSetLayout) {
    pipeline_ = std::make_unique<Pipeline>(
        device_, renderPass_, cameraSetLayout, nullptr,
        "shaders/shadow_depth.vert.spv", "shaders/shadow_depth.frag.spv",
        64,  // push constant size: model matrix (64 bytes)
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_CULL_MODE_FRONT_BIT,  // front culling для shadow bias
        VK_TRUE, VK_TRUE);
    EOA_LOG("Shadow pass pipeline created");
}

void ShadowPass::Resize(uint32_t width, uint32_t height) {
    // Shadow map size is fixed, no resize needed for now
    (void)width;
    (void)height;
}

void ShadowPass::DestroyShadowResources() {
    if (device_ == VK_NULL_HANDLE) return;

    pipeline_.reset();
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    vkDestroyRenderPass(device_, renderPass_, nullptr);
    vkDestroySampler(device_, depthSampler_, nullptr);
    vkDestroyImageView(device_, depthImageView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthImageMemory_, nullptr);
}

} // namespace eoa
