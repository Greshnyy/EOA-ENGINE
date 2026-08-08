#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <map>
#include <filesystem>
#include "renderer/world.h"
#include "renderer/particle_system.h"
#include "renderer/material_graph.h"
#include "renderer/particle_system.h"
#include "renderer/material_graph.h"
#include "core/actor.h"

struct GLFWwindow;
struct ImVec2;
struct ImDrawList;

namespace eoa {

class Editor {
public:
    Editor(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physical,
           VkDevice device, uint32_t graphicsFamily, VkQueue graphicsQueue,
           VkRenderPass renderPass, uint32_t imageCount);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    void NewFrame();
    void Render(VkCommandBuffer cmd);
    bool WantsCaptureMouse() const;
    bool WantsCaptureKeyboard() const;

    World& GetWorld() { return editorWorld_; }
    void SetWorld(World* world) { externalWorld_ = world; }

    void SetOnAssetActivated(std::function<void(const std::string&)> cb) {
        onAssetActivated_ = std::move(cb);
    }

    void Log(const std::string& msg);
    int gizmoMode_ = 0; // 0=Translate, 1=Rotate, 2=Scale

private:
    GLFWwindow* window_;
    VkDevice device_;
    VkDescriptorPool imGuiDescriptorPool_ = VK_NULL_HANDLE;

    World* externalWorld_ = nullptr;
    World editorWorld_;
    std::function<void(const std::string&)> onAssetActivated_;

    // Console
    struct LogEntry { std::string text; int level; }; // 0=info,1=warn,2=error
    std::vector<LogEntry> consoleLog_;
    static constexpr size_t kMaxConsoleLines = 500;
    bool consoleAutoScroll_ = true;
    int consoleFilter_ = 0; // 0=All, 1=Info, 2=Warnings, 3=Errors

    // Hierarchy
    int selectedObjectIndex_ = -1;
    char renameBuf_[128] = {};
    bool renaming_ = false;

    // Content browser
    std::string currentAssetPath_ = "assets/";
    std::vector<std::string> assetEntries_;
    int assetFilter_ = 0; // 0=All, 1=Models, 2=Textures

    // Project panel
    std::vector<std::string> projectTree_;

    // Scene browser
    std::vector<std::string> recentScenes_;

    // Viewport camera
    glm::vec3 vpCameraPos_ = glm::vec3(0, 2, -8);
    float vpCameraYaw_ = 0.0f;
    float vpCameraPitch_ = -15.0f;
    bool vpCameraActive_ = false;
    bool firstFrame_ = true;

    // Gizmo

    bool playing_ = false;
    bool paused_ = false;
    World runtimeWorld_;
    ParticleSystem particleSystem_;
    MaterialGraph materialGraph_;

    // Material editor state
    int matDragNode_ = -1;
    int matLinkFrom_ = -1;
    std::string texViewerPath_;
    int texViewerId_ = 0;
    glm::vec2 matScroll_ = glm::vec2(0);

    // File watcher
    std::map<std::string, std::filesystem::file_time_type> shaderTimestamps_;
    float watcherTimer_ = 0.0f;

    // Init
    void InitImGui(VkInstance instance, VkPhysicalDevice physical, VkDevice device,
                   uint32_t graphicsFamily, VkQueue graphicsQueue,
                   VkRenderPass renderPass, uint32_t imageCount);
    void CreateDescriptorPool();
    void SetupDocking();

    // Panels
    void DrawMainMenuBar();
    void DrawToolbar();
    void DrawViewportPanel();
    void DrawHierarchyPanel();
    void DrawInspectorPanel();
    void DrawContentBrowser();
    void DrawConsolePanel();
    void DrawProjectPanel();
    void DrawStatsOverlay();

    // Helpers
    void RefreshAssetEntries();
    void RefreshProjectTree();
    void SaveScene(const std::string& path);
    void LoadScene(const std::string& path);
    glm::mat4 ViewportViewMatrix() const;
    void HandleViewportInput(const ImVec2& size, const ImVec2& pos);
    void DeleteSelectedActor();
    void DuplicateSelectedActor();
    void DrawInspectorCategory(const char* label, unsigned int color, std::function<void()> body);
    void DrawProfilerOverlay();
    void DrawParticlePanel();
    void DrawMaterialEditorPanel();
    void DrawParticles(ImDrawList* dl, const ImVec2& vpPos, const ImVec2& sz);
    void CheckShaderChanges();
    void DrawTextureViewer();
    std::string OpenFileDialog(const char* filter, const char* title);
    void ImportModel();
    void ImportTexture();
};

} // namespace eoa
