#include "renderer/gbuffer.h"
#include "renderer/buffer_utils.h"
#include "log.h"

namespace eoa {

VkFormat GBuffer::AlbedoFormat() { return VK_FORMAT_R8G8B8A8_SRGB; }
VkFormat GBuffer::NormalFormat() { return VK_FORMAT_R16G16B16A16_SFLOAT; }
VkFormat GBuffer::ORMFormat()    { return VK_FORMAT_R8G8B8A8_UNORM; }

GBuffer::GBuffer(VkPhysicalDevice physical, VkDevice device,
                 uint32_t width, uint32_t height,
                 VkFormat depthFormat)
    : physical_(physical), device_(device), depthFormat_(depthFormat) {
    extent_ = {width, height};
    CreateImages();
    CreateRenderPass();
    CreateFramebuffer();

    // Sampler для чтения GBuffer в Deferred pass
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 1.0f;
    EOA_CHECK_VK(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_));

    EOA_LOG("GBuffer created: %ux%u", width, height);
}

GBuffer::~GBuffer() {
    vkDestroySampler(device_, sampler_, nullptr);
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    vkDestroyRenderPass(device_, renderPass_, nullptr);
    DestroyImages();
}

void GBuffer::CreateImages() {
    VkFormat formats[kColorAttachmentCount] = {
        AlbedoFormat(), NormalFormat(), ORMFormat()
    };
    VkImageView* views[kColorAttachmentCount] = {
        &albedoView_, &normalView_, &ormView_
    };
    VkImage* images[kColorAttachmentCount] = {
        &albedoImage_, &normalImage_, &ormImage_
    };
    VkDeviceMemory* memories[kColorAttachmentCount] = {
        &albedoMemory_, &normalMemory_, &ormMemory_
    };

    for (uint32_t i = 0; i < kColorAttachmentCount; ++i) {
        CreateImage(physical_, device_, extent_.width, extent_.height,
                    formats[i], VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    *images[i], *memories[i]);
        *views[i] = CreateImageView(device_, *images[i], formats[i],
                                      VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // Depth
    CreateImage(physical_, device_, extent_.width, extent_.height,
                depthFormat_, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthImage_, depthMemory_);
    depthView_ = CreateImageView(device_, depthImage_, depthFormat_,
                                  VK_IMAGE_ASPECT_DEPTH_BIT);
}

void GBuffer::DestroyImages() {
    vkDestroyImageView(device_, albedoView_, nullptr);
    vkDestroyImage(device_, albedoImage_, nullptr);
    vkFreeMemory(device_, albedoMemory_, nullptr);

    vkDestroyImageView(device_, normalView_, nullptr);
    vkDestroyImage(device_, normalImage_, nullptr);
    vkFreeMemory(device_, normalMemory_, nullptr);

    vkDestroyImageView(device_, ormView_, nullptr);
    vkDestroyImage(device_, ormImage_, nullptr);
    vkFreeMemory(device_, ormMemory_, nullptr);

    vkDestroyImageView(device_, depthView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthMemory_, nullptr);
}

void GBuffer::CreateRenderPass() {
    VkAttachmentDescription attachments[4]{};

    // Color 0: Albedo
    attachments[0].format = AlbedoFormat();
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Color 1: Normal
    attachments[1].format = NormalFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Color 2: ORM
    attachments[2].format = ORMFormat();
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Depth
    attachments[3].format = depthFormat_;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRefs[3]{};
    colorRefs[0].attachment = 0;
    colorRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorRefs[1].attachment = 1;
    colorRefs[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorRefs[2].attachment = 2;
    colorRefs[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 3;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 3;
    subpass.pColorAttachments = colorRefs;
    subpass.pDepthStencilAttachment = &depthRef;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 4;
    rpInfo.pAttachments = attachments;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;

    EOA_CHECK_VK(vkCreateRenderPass(device_, &rpInfo, nullptr, &renderPass_));
}

void GBuffer::CreateFramebuffer() {
    VkImageView attachments[4] = {
        albedoView_, normalView_, ormView_, depthView_
    };

    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass_;
    fbInfo.attachmentCount = 4;
    fbInfo.pAttachments = attachments;
    fbInfo.width = extent_.width;
    fbInfo.height = extent_.height;
    fbInfo.layers = 1;

    EOA_CHECK_VK(vkCreateFramebuffer(device_, &fbInfo, nullptr, &framebuffer_));
}

void GBuffer::Resize(uint32_t width, uint32_t height) {
    if (width == extent_.width && height == extent_.height) return;
    vkDestroyFramebuffer(device_, framebuffer_, nullptr);
    DestroyImages();
    extent_ = {width, height};
    CreateImages();
    CreateFramebuffer();
    EOA_LOG("GBuffer resized to %ux%u", width, height);
}

} // namespace eoa
