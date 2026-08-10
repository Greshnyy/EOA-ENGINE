#include "editor/editor.h"
#include "log.h"
#include "renderer/vertex.h"
#include "core/transform_component.h"
#include "renderer/mesh_component.h"
#include "renderer/light_component.h"
#include "renderer/camera_component.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <set>
#include <fstream>
#include <ctime>
#include "third_party/imguizmo/ImGuizmo.h"
#include "stb_image.h"

namespace eoa {

namespace {
void CheckVkResult(VkResult err) {
    if (err != VK_SUCCESS) EOA_FATAL("ImGui Vulkan error: VkResult=%d", err);
}
const char* kModelExts[] = {".gltf", ".glb", ".obj", ".fbx"};
const char* kTexExts[] = {".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr"};
bool EndsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}
} // namespace

Editor::Editor(GLFWwindow* w, VkInstance inst, VkPhysicalDevice phys, VkDevice dev,
               uint32_t gfxFam, VkQueue gfxQ, VkRenderPass rp, uint32_t imgCnt)
    : window_(w), device_(dev) {
    InitImGui(inst, phys, dev, gfxFam, gfxQ, rp, imgCnt);
    RefreshAssetEntries();
    RefreshProjectTree();
    EOA_LOG("Editor initialized");
    ParticleEmitter pe;
    pe.name = "default"; pe.spawnRate = 80.0f; pe.lifetime = 2.5f;
    pe.startSize = 0.08f; pe.endSize = 0.01f;
    pe.startColor = glm::vec4(0.2f,0.6f,1.0f,1.0f);
    pe.endColor = glm::vec4(1.0f,0.3f,0.1f,0.0f);
    pe.velocity = glm::vec3(0,0.3f,0); pe.velocityRandom = 1.0f;
    pe.gravity = glm::vec3(0,-0.15f,0); pe.maxParticles = 5000;
    particleSystem_.AddEmitter(pe);
}

Editor::~Editor() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    if (imGuiDescriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, imGuiDescriptorPool_, nullptr);
        imGuiDescriptorPool_ = VK_NULL_HANDLE;
    }
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::DestroyContext();
    }
}

void Editor::InitImGui(VkInstance inst, VkPhysicalDevice phys, VkDevice dev,
                       uint32_t gfxFam, VkQueue gfxQ, VkRenderPass rp, uint32_t imgCnt) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 3; s.FrameRounding = 2;
    s.WindowPadding = ImVec2(4,4); s.FramePadding = ImVec2(6,3); s.ItemSpacing = ImVec2(4,4);
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f,0.06f,0.06f,1);
    s.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f,0.08f,0.08f,1);
    s.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f,0.12f,0.12f,1);
    s.Colors[ImGuiCol_Header] = ImVec4(0.14f,0.14f,0.14f,0.55f);
    s.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f,0.20f,0.20f,0.8f);
    s.Colors[ImGuiCol_HeaderActive] = ImVec4(0.24f,0.24f,0.24f,1);
    s.Colors[ImGuiCol_Button] = ImVec4(0.14f,0.14f,0.14f,0.6f);
    s.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f,0.22f,0.22f,0.8f);
    s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.28f,0.28f,0.28f,1);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(0.09f,0.09f,0.09f,0.54f);
    s.Colors[ImGuiCol_Text] = ImVec4(0.82f,0.82f,0.82f,1);
    s.Colors[ImGuiCol_Border] = ImVec4(0.16f,0.16f,0.16f,0.5f);
    s.Colors[ImGuiCol_Separator] = ImVec4(0.16f,0.16f,0.16f,0.6f);
    s.Colors[ImGuiCol_Tab] = ImVec4(0.07f,0.07f,0.07f,1);
    s.Colors[ImGuiCol_TabHovered] = ImVec4(0.16f,0.16f,0.16f,1);
    s.Colors[ImGuiCol_TabActive] = ImVec4(0.12f,0.12f,0.12f,1);

    ImGui_ImplGlfw_InitForVulkan(window_, true);
    CreateDescriptorPool();

    ImGui_ImplVulkan_InitInfo ii{};
    ii.Instance=inst; ii.PhysicalDevice=phys; ii.Device=dev;
    ii.QueueFamily=gfxFam; ii.Queue=gfxQ;
    ii.PipelineCache=VK_NULL_HANDLE; ii.DescriptorPool=imGuiDescriptorPool_;
    ii.MinImageCount=imgCnt; ii.ImageCount=imgCnt;
    ii.Allocator=nullptr; ii.CheckVkResultFn=CheckVkResult;
    ii.PipelineInfoMain.RenderPass=rp; ii.PipelineInfoMain.Subpass=0;
    ii.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&ii);
}

void Editor::CreateDescriptorPool() {
    VkDescriptorPoolSize ps[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER,1000},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1000},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,1000},{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1000},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,1000},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,1000},
    };
    VkDescriptorPoolCreateInfo pi{};
    pi.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pi.maxSets=1000; pi.poolSizeCount=11; pi.pPoolSizes=ps;
    EOA_CHECK_VK(vkCreateDescriptorPool(device_,&pi,nullptr,&imGuiDescriptorPool_));
}

void Editor::SetupDocking() {
    ImGuiID ds = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    if (firstFrame_) {
        firstFrame_ = false;
        ImGui::DockBuilderRemoveNode(ds);
        ImGui::DockBuilderAddNode(ds, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(ds, ImGui::GetMainViewport()->Size);
        ImGuiID dRight = ImGui::DockBuilderSplitNode(ds, ImGuiDir_Right, 0.22f, nullptr, &ds);
        ImGuiID dLeft = ImGui::DockBuilderSplitNode(ds, ImGuiDir_Left, 0.18f, nullptr, &ds);
        ImGuiID dBot = ImGui::DockBuilderSplitNode(ds, ImGuiDir_Down, 0.26f, nullptr, &ds);
        ImGui::DockBuilderDockWindow("Project", dLeft);
        ImGui::DockBuilderDockWindow("Hierarchy", dLeft);
        ImGui::DockBuilderDockWindow("Inspector", dRight);
        ImGui::DockBuilderDockWindow("Content Browser", dBot);
        ImGui::DockBuilderDockWindow("Console", dBot);
        ImGui::DockBuilderDockWindow("Viewport", ds);
        ImGui::DockBuilderFinish(ds);
    }
}

void Editor::NewFrame() {
    ImGui_ImplVulkan_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
    SetupDocking();
    static auto plast = std::chrono::steady_clock::now();
    auto pnow = std::chrono::steady_clock::now();
    float pdt = std::chrono::duration<float>(pnow - plast).count();
    plast = pnow;
    if (!particleSystem_.GetEmitters().empty()) particleSystem_.Update(pdt, vpCameraPos_);
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoMode_ = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoMode_ = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoMode_ = 2;
    }
    DrawMainMenuBar(); DrawToolbar(); CheckShaderChanges();
    DrawProjectPanel(); DrawViewportPanel(); DrawHierarchyPanel();
    DrawInspectorPanel(); DrawContentBrowser(); DrawConsolePanel(); DrawStatsOverlay();
}

void Editor::Render(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

bool Editor::WantsCaptureMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool Editor::WantsCaptureKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

void Editor::Log(const std::string& msg) {
    int lv = 0;
    if (msg.find("[ERROR]")!=std::string::npos||msg.find("[FATAL]")!=std::string::npos) lv=2;
    else if (msg.find("[WARN]")!=std::string::npos) lv=1;
    auto t = std::time(nullptr);
    char tb[16]; struct tm tmb; localtime_s(&t,&t); strftime(tb,sizeof(tb),"%H:%M:%S",&tmb);
    consoleLog_.push_back({std::string(tb)+" "+msg, lv});
    if (consoleLog_.size()>kMaxConsoleLines) consoleLog_.erase(consoleLog_.begin());
}
