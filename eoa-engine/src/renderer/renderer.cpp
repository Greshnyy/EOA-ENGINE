#include "renderer/renderer.h"
#include "renderer/gbuffer.h"
#include "renderer/deferred_lighting.h"
#include "renderer/buffer_utils.h"
#include "renderer/vertex.h"
#include "renderer/gltf_loader.h"
#include "editor/editor.h"
#include "log.h"
#include <chrono>
#include <cstring>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace eoa {

Renderer::Renderer(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface,
                    uint32_t graphicsFamily, VkQueue graphicsQueue,
                    uint32_t width, uint32_t height)
    : physical_(physical), device_(device), surface_(surface),
      graphicsFamily_(graphicsFamily), graphicsQueue_(graphicsQueue) {
    depthFormat_ = FindDepthFormat();

    swapchain_ = std::make_unique<VulkanSwapchain>(physical_, device_, surface_,
                                                     graphicsFamily_, width, height);
    CreateRenderPass();
    CreateDepthResources(width, height);
    CreateDescriptorSetLayouts();
    CreatePipeline();
    CreateFramebuffers();

    // Deferred rendering setup
    gbuffer_ = std::make_unique<GBuffer>(physical_, device_, width, height, depthFormat_);
    CreateGBufferPipeline();
    CreateDeferredLighting();
    CreateLightBuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateCameraDescriptorSets();
    CreateDefaultMaterial();
    CreateTestWorld();
    CreateSyncObjects();
}

VkFormat Renderer::FindDepthFormat() const {
    VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D24_UNORM_S8_UINT};
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_, format, &props);
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }
    EOA_FATAL("No suitable depth format found");
}

void Renderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchain_->ImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat_;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 2;
    info.pAttachments = attachments;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    EOA_CHECK_VK(vkCreateRenderPass(device_, &info, nullptr, &renderPass_));
}

void Renderer::CreateDepthResources(uint32_t width, uint32_t height) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat_;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
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

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    EOA_CHECK_VK(vkCreateImageView(device_, &viewInfo, nullptr, &depthImageView_));
}

void Renderer::CreateDescriptorSetLayouts() {
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo cameraInfo{};
    cameraInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraInfo.bindingCount = 1;
    cameraInfo.pBindings = &uboBinding;
    EOA_CHECK_VK(vkCreateDescriptorSetLayout(device_, &cameraInfo, nullptr, &cameraSetLayout_));

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo materialInfo{};
    materialInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialInfo.bindingCount = 1;
    materialInfo.pBindings = &samplerBinding;
    EOA_CHECK_VK(vkCreateDescriptorSetLayout(device_, &materialInfo, nullptr, &materialSetLayout_));
}

void Renderer::CreatePipeline() {
    pipeline_ = std::make_unique<Pipeline>(device_, renderPass_, cameraSetLayout_, materialSetLayout_,
                                            "shaders/triangle.vert.spv",
                                            "shaders/triangle.frag.spv");
}

void Renderer::CreateFramebuffers() {
    framebuffers_.resize(swapchain_->ImageViews().size());
    for (size_t i = 0; i < framebuffers_.size(); ++i) {
        VkImageView attachments[] = {swapchain_->ImageViews()[i], depthImageView_};

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = renderPass_;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.width = swapchain_->Extent().width;
        info.height = swapchain_->Extent().height;
        info.layers = 1;

        EOA_CHECK_VK(vkCreateFramebuffer(device_, &info, nullptr, &framebuffers_[i]));
    }
}

void Renderer::CreateCommandPool() {
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = graphicsFamily_;
    EOA_CHECK_VK(vkCreateCommandPool(device_, &info, nullptr, &commandPool_));
}

void Renderer::CreateCommandBuffers() {
    commandBuffers_.resize(kFramesInFlight);
    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = commandPool_;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = kFramesInFlight;
    EOA_CHECK_VK(vkAllocateCommandBuffers(device_, &info, commandBuffers_.data()));
}

void Renderer::CreateUniformBuffers() {
    VkDeviceSize size = sizeof(UniformBufferObject);
    uniformBuffers_.resize(kFramesInFlight);
    uniformBuffersMemory_.resize(kFramesInFlight);
    uniformBuffersMapped_.resize(kFramesInFlight);

    for (int i = 0; i < kFramesInFlight; ++i) {
        CreateBuffer(physical_, device_, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers_[i], uniformBuffersMemory_[i]);
        vkMapMemory(device_, uniformBuffersMemory_[i], 0, size, 0, &uniformBuffersMapped_[i]);
    }
}

void Renderer::CreateDescriptorPool() {
    // maxSets: kFramesInFlight под камеру + kMaxMaterials под текстуры материалов.
    VkDescriptorPoolSize poolSizes[2]{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = kFramesInFlight;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = kMaxMaterials;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = kFramesInFlight + kMaxMaterials;

    EOA_CHECK_VK(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_));
}

void Renderer::CreateCameraDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(kFramesInFlight, cameraSetLayout_);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = kFramesInFlight;
    allocInfo.pSetLayouts = layouts.data();

    cameraDescriptorSets_.resize(kFramesInFlight);
    EOA_CHECK_VK(vkAllocateDescriptorSets(device_, &allocInfo, cameraDescriptorSets_.data()));

    for (int i = 0; i < kFramesInFlight; ++i) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers_[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = cameraDescriptorSets_[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
}

std::shared_ptr<MaterialData> Renderer::CreateMaterialWithTexture(const std::string& name,
                                                                 const std::string& texturePath) {
    auto material = std::make_shared<MaterialData>();
    material->name = name;
    material->texture = std::make_unique<Texture>(physical_, device_, commandPool_,
                                                     graphicsQueue_, texturePath);

    VkDescriptorSetLayout layout = materialSetLayout_;
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    EOA_CHECK_VK(vkAllocateDescriptorSets(device_, &allocInfo, &material->descriptorSet));

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = material->texture->View();
    imageInfo.sampler = material->texture->Sampler();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = material->descriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    return material;
}

void Renderer::CreateDefaultMaterial() {
    defaultMaterial_ = CreateMaterialWithTexture("default_checker", "assets/textures/checker.png");
}

void Renderer::LoadGltfIntoWorld(const std::string& path) {
    GltfModel gltf = GltfLoader::Load(path);
    if (gltf.meshVertices.empty()) {
        EOA_WARN("glTF '%s' не дал ни одного меша — объект не добавлен", path.c_str());
        return;
    }

    size_t slashPos = path.find_last_of("/\\");
    std::string fileName = (slashPos == std::string::npos) ? path : path.substr(slashPos + 1);
    size_t dotPos = fileName.find_last_of('.');
    std::string baseName = (dotPos == std::string::npos) ? fileName : fileName.substr(0, dotPos);

    for (size_t mi = 0; mi < gltf.meshVertices.size(); ++mi) {
        if (gltf.meshVertices[mi].empty()) continue;

        std::string objName = gltf.meshVertices.size() > 1 ?
            baseName + "_" + std::to_string(mi) : baseName;

        auto* actor = world_.SpawnActor(objName);
        actor->AddComponent<TransformComponent>("Transform");
        auto* meshComp = actor->AddComponent<MeshComponent>(
            physical_, device_, commandPool_, graphicsQueue_, "Mesh");

        meshComp->SetMesh(std::make_unique<Mesh>(physical_, device_, commandPool_,
            graphicsQueue_, gltf.meshVertices[mi], gltf.meshIndices[mi]));

        int matIdx = (mi < static_cast<int>(gltf.meshMaterialIndex.size())) ? gltf.meshMaterialIndex[mi] : -1;
        if (matIdx >= 0 && matIdx < static_cast<int>(gltf.materials.size()) &&
            !gltf.materials[matIdx].albedoTexturePath.empty()) {
            meshComp->SetMaterial(CreateMaterialWithTexture(
                gltf.materials[matIdx].name.empty() ? (baseName + "_material") : gltf.materials[matIdx].name,
                gltf.materials[matIdx].albedoTexturePath));
        } else {
            meshComp->SetMaterial(defaultMaterial_);
        }
    }

    if (editor_) {
        editor_->Log("Loaded: " + path);
    }
    EOA_LOG("glTF added to world: %s", path.c_str());
  }

  void Renderer::CreateTestWorld() {
    std::vector<Vertex> floorVerts = {
        {{-3.0f, -1.5f, 2.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.45f, 0.35f}, {0.0f, 0.0f}},
        {{ 3.0f, -1.5f, 2.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.45f, 0.35f}, {4.0f, 0.0f}},
        {{ 3.0f,  1.5f, 2.0f}, {0.0f, 1.0f, 0.0f}, {0.55f, 0.48f, 0.38f}, {4.0f, 4.0f}},
        {{-3.0f,  1.5f, 2.0f}, {0.0f, 1.0f, 0.0f}, {0.55f, 0.48f, 0.38f}, {0.0f, 4.0f}},
    };
    std::vector<uint16_t> floorIndices = {0, 1, 2, 2, 3, 0};

        auto* floorActor = world_.SpawnActor("floor");
    floorActor->AddComponent<TransformComponent>("Transform");
    auto* floorMesh = floorActor->AddComponent<MeshComponent>(
        physical_, device_, commandPool_, graphicsQueue_, "Mesh");
    floorMesh->SetMesh(std::make_unique<Mesh>(physical_, device_, commandPool_,
        graphicsQueue_, floorVerts, floorIndices));
    floorMesh->SetMaterial(defaultMaterial_);
    floorActor->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, -1.0f, 0.0f));

    std::vector<Vertex> triVerts = {
        {{ 0.0f, -0.7f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.85f, 0.35f, 0.10f}, {0.5f, 0.0f}},
        {{ 0.8f,  0.7f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.20f, 0.55f, 0.20f}, {1.0f, 1.0f}},
        {{-0.8f,  0.7f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.70f, 0.65f, 0.55f}, {0.0f, 1.0f}},
    };
    std::vector<uint16_t> triIndices = {0, 1, 2};

        auto* triActor = world_.SpawnActor("triangle");
    triActor->AddComponent<TransformComponent>("Transform");
    auto* triMesh = triActor->AddComponent<MeshComponent>(
        physical_, device_, commandPool_, graphicsQueue_, "Mesh");
        triMesh->SetMesh(std::make_unique<Mesh>(physical_, device_, commandPool_, graphicsQueue_,
                                         triVerts, triIndices));
    triMesh->SetMaterial(defaultMaterial_);
    triActor->GetComponent<TransformComponent>()->SetPosition(glm::vec3(0.0f, 0.5f, 0.0f));

    std::vector<Vertex> cubeVerts = {
        // front
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.7f, 0.6f, 0.4f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.7f, 0.6f, 0.4f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.7f, 0.6f, 0.4f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.7f, 0.6f, 0.4f}, {0.0f, 1.0f}},
        // back
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.6f, 0.5f, 0.35f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.6f, 0.5f, 0.35f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.6f, 0.5f, 0.35f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.6f, 0.5f, 0.35f}, {0.0f, 1.0f}},
    };
    std::vector<uint16_t> cubeIndices = {
        0,1,2, 2,3,0,   // front
        5,4,7, 7,6,5,   // back
        4,0,3, 3,7,4,   // left
        1,5,6, 6,2,1,   // right
        3,2,6, 6,7,3,   // top
        4,5,1, 1,0,4,   // bottom
    };

        auto* cubeActor = world_.SpawnActor("cube");
    cubeActor->AddComponent<TransformComponent>("Transform");
    auto* cubeMesh = cubeActor->AddComponent<MeshComponent>(
        physical_, device_, commandPool_, graphicsQueue_, "Mesh");
        cubeMesh->SetMesh(std::make_unique<Mesh>(physical_, device_, commandPool_, graphicsQueue_,
                                          cubeVerts, cubeIndices));
    cubeMesh->SetMaterial(defaultMaterial_);
    auto* cubeT = cubeActor->GetComponent<TransformComponent>();
    cubeT->SetPosition(glm::vec3(-1.5f, 0.0f, 0.0f));
    cubeT->SetScale(glm::vec3(0.6f));

        auto* cube2Actor = world_.SpawnActor("cube2");
    cube2Actor->AddComponent<TransformComponent>("Transform");
    auto* cube2Mesh = cube2Actor->AddComponent<MeshComponent>(
        physical_, device_, commandPool_, graphicsQueue_, "Mesh");
        cube2Mesh->SetMesh(std::make_unique<Mesh>(physical_, device_, commandPool_, graphicsQueue_,
                                           cubeVerts, cubeIndices));
    cube2Mesh->SetMaterial(defaultMaterial_);
    auto* cube2T = cube2Actor->GetComponent<TransformComponent>();
    cube2T->SetPosition(glm::vec3(1.5f, 0.0f, 0.0f));
    cube2T->SetScale(glm::vec3(0.6f));

    // Первый объект, реально прошедший через GltfLoader (не хардкод-геометрия
    // выше) — доказательство, что загрузчик работает end-to-end.
    LoadGltfIntoWorld("assets/models/test_pyramid.gltf");
    if (!world_.actors.empty()) {
        auto* pyrT = world_.actors.back()->GetComponent<TransformComponent>();
        if (pyrT) pyrT->SetPosition(glm::vec3(0.0f, 1.8f, 0.0f));
    }
}

void Renderer::CreateSyncObjects() {
    imageAvailable_.resize(kFramesInFlight);
    renderFinished_.resize(kFramesInFlight);
    inFlightFences_.resize(kFramesInFlight);

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < kFramesInFlight; ++i) {
        EOA_CHECK_VK(vkCreateSemaphore(device_, &semInfo, nullptr, &imageAvailable_[i]));
        EOA_CHECK_VK(vkCreateSemaphore(device_, &semInfo, nullptr, &renderFinished_[i]));
        EOA_CHECK_VK(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]));
    }
}

void Renderer::UpdateUniformBuffer(uint32_t frameIndex, uint32_t width, uint32_t height,
                                     const glm::mat4& viewMatrix) {
    UniformBufferObject ubo{};
    ubo.view = viewMatrix;
    ubo.proj = glm::perspective(glm::radians(60.0f),
                                 static_cast<float>(width) / static_cast<float>(height),
                                 0.1f, 100.0f);
    ubo.proj[1][1] *= -1.0f;
    ubo.lightDir = glm::vec4(glm::normalize(glm::vec3(0.5f, -1.0f, -0.3f)), 1.5f);
    ubo.lightColor = glm::vec4(1.0f, 0.95f, 0.85f, 0.0f);

    std::memcpy(uniformBuffersMapped_[frameIndex], &ubo, sizeof(ubo));
}

void Renderer::RecordGBufferPass(VkCommandBuffer cmd) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    EOA_CHECK_VK(vkBeginCommandBuffer(cmd, &beginInfo));

    // -- GBuffer fill pass --
    VkClearValue clearValues[4];
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};  // Albedo
    clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}};  // Normal
    clearValues[2].color = {{1.0f, 0.0f, 0.0f, 0.0f}};  // ORM (roughness=1, metallic=0)
    clearValues[3].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = gbuffer_->GetRenderPass();
    rpInfo.framebuffer = gbuffer_->GetFramebuffer();
    rpInfo.renderArea.extent = gbuffer_->GetExtent();
    rpInfo.clearValueCount = 4;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline_->Handle());

    VkViewport viewport{};
    viewport.width = static_cast<float>(gbuffer_->GetExtent().width);
    viewport.height = static_cast<float>(gbuffer_->GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, gbuffer_->GetExtent()};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline_->Layout(),
                             0, 1, &cameraDescriptorSets_[currentFrame_], 0, nullptr);

    for (auto& actor : world_.actors) {
        if (!actor->IsVisible()) continue;
        auto* meshComp = actor->GetComponent<MeshComponent>();
        if (!meshComp || !meshComp->HasMesh()) continue;

        auto matData = meshComp->GetMaterial();
        MaterialData* mat = matData ? matData.get() : defaultMaterial_.get();
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline_->Layout(),
                                 1, 1, &mat->descriptorSet, 0, nullptr);

        meshComp->BindDraw(cmd, gbufferPipeline_->Layout());
    }

    vkCmdEndRenderPass(cmd);
}

void Renderer::RecordDeferredPass(VkCommandBuffer cmd, uint32_t imageIndex) {
    // -- Deferred lighting pass → swapchain --
    VkClearValue clearValue{};
    clearValue.color = {{0.06f, 0.05f, 0.04f, 1.0f}};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = renderPass_;
    rpInfo.framebuffer = framebuffers_[imageIndex];
    rpInfo.renderArea.extent = swapchain_->Extent();
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLighting_->GetPipeline());

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain_->Extent().width);
    viewport.height = static_cast<float>(swapchain_->Extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, swapchain_->Extent()};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

      VkDescriptorSet ds = deferredLighting_->GetDescriptorSet();
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, deferredLighting_->GetLayout(),
                               0, 1, &ds, 0, nullptr);

    vkCmdDraw(cmd, 3, 1, 0, 0);  // Fullscreen triangle

    if (editor_) {
        editor_->Render(cmd);
    }

    vkCmdEndRenderPass(cmd);
    EOA_CHECK_VK(vkEndCommandBuffer(cmd));
}

void Renderer::CleanupSwapchainDependents() {
    vkDestroyImageView(device_, depthImageView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthImageMemory_, nullptr);

    for (auto fb : framebuffers_) vkDestroyFramebuffer(device_, fb, nullptr);
    framebuffers_.clear();
    // Destroy GBuffer pipeline (needs old renderPass refs gone first)
    gbufferPipeline_.reset();
    deferredLighting_.reset();

    vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;
}

void Renderer::RecreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device_);

    CleanupSwapchainDependents();
    swapchain_.reset();

    swapchain_ = std::make_unique<VulkanSwapchain>(physical_, device_, surface_,
                                                     graphicsFamily_, width, height);
    CreateRenderPass();
    CreateDepthResources(width, height);
    CreateFramebuffers();

    // Recreate deferred structures with new size
    gbuffer_->Resize(width, height);
    CreateGBufferPipeline();
    CreateDeferredLighting();
    EOA_LOG("Swapchain recreated %ux%u", width, height);
}

void Renderer::DrawFrame(uint32_t currentWidth, uint32_t currentHeight, const glm::mat4& viewMatrix) {
    if (currentWidth == 0 || currentHeight == 0) {
        return;
    }

    vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        device_, swapchain_->Handle(), UINT64_MAX,
        imageAvailable_[currentFrame_], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain(currentWidth, currentHeight);
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        EOA_FATAL("vkAcquireNextImageKHR failed: VkResult=%d", acquireResult);
    }

    UpdateUniformBuffer(static_cast<uint32_t>(currentFrame_), currentWidth, currentHeight,
                          viewMatrix);

    vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

    VkCommandBuffer cmd = commandBuffers_[currentFrame_];
    vkResetCommandBuffer(cmd, 0);
    UpdateLightBuffers();
    RecordGBufferPass(cmd);
    RecordDeferredPass(cmd, imageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailable_[currentFrame_];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinished_[currentFrame_];

    EOA_CHECK_VK(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished_[currentFrame_];
    VkSwapchainKHR swapchains[] = {swapchain_->Handle()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue_, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain(currentWidth, currentHeight);
    } else if (presentResult != VK_SUCCESS) {
        EOA_FATAL("vkQueuePresentKHR failed: VkResult=%d", presentResult);
    }

    currentFrame_ = (currentFrame_ + 1) % kFramesInFlight;
}

Renderer::~Renderer() {
    vkDeviceWaitIdle(device_);

    world_.Clear();
    defaultMaterial_.reset();

    vkDestroyBuffer(device_, lightArrayBuffer_, nullptr);
    vkFreeMemory(device_, lightArrayBufferMemory_, nullptr);
    vkDestroyBuffer(device_, lightInfoBuffer_, nullptr);
    vkFreeMemory(device_, lightInfoBufferMemory_, nullptr);

    deferredLighting_.reset();
    gbufferPipeline_.reset();
    gbuffer_.reset();

    for (int i = 0; i < kFramesInFlight; ++i) {
        vkDestroySemaphore(device_, imageAvailable_[i], nullptr);
        vkDestroySemaphore(device_, renderFinished_[i], nullptr);
        vkDestroyFence(device_, inFlightFences_[i], nullptr);
    }

    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

    for (int i = 0; i < kFramesInFlight; ++i) {
        vkUnmapMemory(device_, uniformBuffersMemory_[i]);
        vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
        vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
    }

    vkDestroyCommandPool(device_, commandPool_, nullptr);

    pipeline_.reset();
    vkDestroyDescriptorSetLayout(device_, materialSetLayout_, nullptr);
    vkDestroyDescriptorSetLayout(device_, cameraSetLayout_, nullptr);

    CleanupSwapchainDependents();
}


void Renderer::CreateGBufferPipeline() {
    gbufferPipeline_ = std::make_unique<Pipeline>(
        device_, gbuffer_->GetRenderPass(), cameraSetLayout_, materialSetLayout_,
        "shaders/gbuffer.vert.spv", "shaders/gbuffer.frag.spv",
        96,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_CULL_MODE_BACK_BIT, VK_TRUE, VK_TRUE);
    EOA_LOG("GBuffer pipeline created");
}

void Renderer::CreateDeferredLighting() {
    deferredLighting_.reset();
    deferredLighting_ = std::make_unique<DeferredLighting>(
        physical_, device_, renderPass_, 0);
    deferredLighting_->CreatePipeline(cameraSetLayout_, materialSetLayout_,
                                       "shaders/deferred_lighting.vert.spv",
                                       "shaders/deferred_lighting.frag.spv");
    deferredLighting_->UpdateDescriptorSet(*gbuffer_);
}

void Renderer::CreateLightBuffers() {
    // Light info UBO (camera pos + ambient + light count)
    VkDeviceSize infoSize = sizeof(glm::vec4) * 3;  // 48 bytes, aligned
    CreateBuffer(physical_, device_, infoSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 lightInfoBuffer_, lightInfoBufferMemory_);

    // Light array SSBO (64 lights max)
    VkDeviceSize arraySize = 64 * (sizeof(glm::vec4) * 4);  // 64 lights * 64 bytes each = 4KB
    CreateBuffer(physical_, device_, arraySize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 lightArrayBuffer_, lightArrayBufferMemory_);

    EOA_LOG("Light buffers created");
}

void Renderer::UpdateLightBuffers() {
    // Collect lights from World actors
    struct LightGPU {
        glm::vec4 color_intensity;  // .rgb = color, .a = intensity
        glm::vec4 pos_range;         // .xyz = position, .w = range
        glm::vec4 dir_type;          // .xyz = direction, .w = type
        glm::vec4 cone_params;       // .x = inner cos, .y = outer cos
    };

    std::vector<LightGPU> lights;
    glm::vec3 camPos(0.0f);

    for (auto& actor : world_.actors) {
        auto* camComp = actor->GetComponent<CameraComponent>();
        if (camComp) {
            auto* t = actor->GetComponent<TransformComponent>();
            if (t) camPos = t->GetPosition();
            break;  // use first camera found
        }
    }

    for (auto& actor : world_.actors) {
        auto* lightComp = actor->GetComponent<LightComponent>();
        if (!lightComp) continue;

        auto* t = actor->GetComponent<TransformComponent>();
        glm::vec3 pos = t ? t->GetPosition() : glm::vec3(0.0f);
        glm::vec3 dir = t ? t->Forward() : glm::vec3(0.0f, -1.0f, 0.0f);

        LightGPU gpuLight{};
        gpuLight.color_intensity = glm::vec4(lightComp->GetColor(), lightComp->GetIntensity());
        gpuLight.pos_range = glm::vec4(pos, lightComp->GetRange());
        gpuLight.dir_type = glm::vec4(dir, static_cast<float>(static_cast<uint32_t>(lightComp->GetLightType())));
        gpuLight.cone_params = glm::vec4(
            glm::cos(glm::radians(lightComp->GetInnerConeAngle())),
            glm::cos(glm::radians(lightComp->GetOuterConeAngle())),
            0.0f, 0.0f);
        lights.push_back(gpuLight);
    }

    if (lights.empty()) {
        // Add default directional light
        LightGPU sun;
        sun.color_intensity = glm::vec4(1.0f, 0.95f, 0.85f, 1.5f);
        sun.pos_range = glm::vec4(0.0f, 10.0f, 0.0f, 1.0f);
        sun.dir_type = glm::vec4(glm::normalize(glm::vec3(0.5f, -1.0f, -0.3f)), 0.0f);
        sun.cone_params = glm::vec4(0.0f);
        lights.push_back(sun);
    }

    // Update light info UBO
    struct LightInfo {
        glm::vec4 camPos;       // .xyz = camera pos
        glm::vec4 ambientColor; // .rgb = ambient
        glm::uvec4 params;      // .x = lightCount
    } lightInfo;

    lightInfo.camPos = glm::vec4(camPos, 0.0f);
    lightInfo.ambientColor = glm::vec4(0.08f, 0.07f, 0.06f, 0.0f);
    lightInfo.params = glm::uvec4(static_cast<uint32_t>(lights.size()), 0, 0, 0);

    void* mappedData;
    vkMapMemory(device_, lightInfoBufferMemory_, 0, sizeof(lightInfo), 0, &mappedData);
    memcpy(mappedData, &lightInfo, sizeof(lightInfo));
    vkUnmapMemory(device_, lightInfoBufferMemory_);

    // Update light array SSBO
    VkDeviceSize arraySize = lights.size() * sizeof(LightGPU);
    vkMapMemory(device_, lightArrayBufferMemory_, 0, arraySize, 0, &mappedData);
    memcpy(mappedData, lights.data(), arraySize);
    vkUnmapMemory(device_, lightArrayBufferMemory_);
}


} // namespace eoa
