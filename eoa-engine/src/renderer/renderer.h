#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include "rhi/vk_swapchain.h"
#include "renderer/pipeline.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"
#include "renderer/world.h"
#include "core/actor.h"
#include "core/transform_component.h"
#include "renderer/mesh_component.h"
#include "renderer/camera_component.h"
#include "renderer/light_component.h"
#include "renderer/gbuffer.h"
#include "renderer/deferred_lighting.h"

namespace eoa {

class Editor;

class Renderer {
public:
    Renderer(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface,
              uint32_t graphicsFamily, VkQueue graphicsQueue,
              uint32_t width, uint32_t height);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void DrawFrame(uint32_t currentWidth, uint32_t currentHeight, const glm::mat4& viewMatrix);
    World& GetWorld() { return world_; }
    VkRenderPass GetRenderPass() const { return renderPass_; }
    VkRenderPass GetGBufferRenderPass() const { return gbuffer_->GetRenderPass(); }
    void SetEditor(Editor* editor) { editor_ = editor; }

    // Грузит .gltf/.glb с диска и добавляет как новый объект в сцену. Публичный —
    // вызывается и из CreateTestScene() при старте, и из Editor (Asset Browser)
    // по двойному клику на файл. Материал берётся из glTF, если там есть текстура;
    // иначе — общий defaultMaterial_.
    void LoadGltfIntoWorld(const std::string& path);

private:
    static constexpr int kFramesInFlight = 3;
    // Сколько материалов (с уникальными текстурами) движок готов держать
    // одновременно. Статический лимит на размер descriptor pool — упрощение
    // на сейчас; вырастет в проблему только если Asset Browser зафлудить
    // загрузкой 60+ разных текстурированных моделей подряд без выгрузки сцены.
    static constexpr uint32_t kMaxMaterials = 64;

    VkPhysicalDevice physical_;
    VkDevice device_;
    VkSurfaceKHR surface_;
    uint32_t graphicsFamily_;
    VkQueue graphicsQueue_;

    std::unique_ptr<VulkanSwapchain> swapchain_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;
    VkFormat depthFormat_;

    // Два набора: set=0 — камера/свет (общий на кадр, kFramesInFlight штук),
    // set=1 — текстура материала (свой на Material, не на кадр). См. комментарий
    // в pipeline.h почему они разделены.
    VkDescriptorSetLayout cameraSetLayout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout materialSetLayout_ = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> pipeline_;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceMemory> uniformBuffersMemory_;
    std::vector<void*> uniformBuffersMapped_;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> cameraDescriptorSets_; // по одному на frame-in-flight

    World world_;
    std::shared_ptr<MaterialData> defaultMaterial_;
    Editor* editor_ = nullptr;

    // Deferred rendering
    std::unique_ptr<GBuffer> gbuffer_;
    std::unique_ptr<DeferredLighting> deferredLighting_;
    std::unique_ptr<Pipeline> gbufferPipeline_;

    // Light uniform buffers
    VkBuffer lightArrayBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory lightArrayBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer lightInfoBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory lightInfoBufferMemory_ = VK_NULL_HANDLE;

    std::vector<VkSemaphore> imageAvailable_;
    std::vector<VkSemaphore> renderFinished_;
    std::vector<VkFence> inFlightFences_;
    size_t currentFrame_ = 0;

    // Deferred rendering setup
    void CreateGBufferPipeline();
    void CreateDeferredLighting();
    void CreateLightBuffers();
    void UpdateLightBuffers();
    void RecordGBufferPass(VkCommandBuffer cmd);
    void RecordDeferredPass(VkCommandBuffer cmd, uint32_t imageIndex);

    void CreateRenderPass();
    void CreateDepthResources(uint32_t width, uint32_t height);
    void CreateDescriptorSetLayouts();
    void CreatePipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateCameraDescriptorSets();
    void CreateDefaultMaterial();
    void CreateTestWorld();
    void CreateSyncObjects();
    void UpdateUniformBuffer(uint32_t frameIndex, uint32_t width, uint32_t height,
                               const glm::mat4& viewMatrix);
    void RecordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void RecreateSwapchain(uint32_t width, uint32_t height);
    void CleanupSwapchainDependents();
    VkFormat FindDepthFormat() const;

    // Создаёt Material с собственным Texture + descriptor set (set=1) под неё.
    // Аллоцирует из общего descriptorPool_ — вызывать только пока не исчерпан kMaxMaterials.
    std::shared_ptr<MaterialData> CreateMaterialWithTexture(const std::string& name,
                                                           const std::string& texturePath);
};

} // namespace eoa
