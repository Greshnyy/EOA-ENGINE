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

// ===================================================================
// Init / NewFrame / Helpers
// ===================================================================
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
    ImGuiID ds = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode);
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
    // Particle update
    static auto plast = std::chrono::steady_clock::now();
    auto pnow = std::chrono::steady_clock::now();
    float pdt = std::chrono::duration<float>(pnow - plast).count();
    plast = pnow;
    if (!particleSystem_.GetEmitters().empty())
        particleSystem_.Update(pdt, vpCameraPos_);
    // Gizmo hotkeys (W/E/R) - only when not typing in text fields
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoMode_ = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoMode_ = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoMode_ = 2;
    }
    DrawMainMenuBar(); DrawToolbar();
    CheckShaderChanges();
    DrawProjectPanel(); DrawViewportPanel(); DrawHierarchyPanel();
    DrawInspectorPanel(); DrawContentBrowser(); DrawConsolePanel();
    DrawStatsOverlay();
}

void Editor::Render(VkCommandBuffer cmd) {
    ImGui::Render(); ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
bool Editor::WantsCaptureMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool Editor::WantsCaptureKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

void Editor::Log(const std::string& msg) {
    int lv = 0;
    if (msg.find("[ERROR]")!=std::string::npos||msg.find("[FATAL]")!=std::string::npos) lv=2;
    else if (msg.find("[WARN]")!=std::string::npos) lv=1;
    auto t = std::time(nullptr);
    char tb[16]; struct tm tmb; localtime_s(&tmb,&t); strftime(tb,sizeof(tb),"%H:%M:%S",&tmb);
    consoleLog_.push_back({std::string(tb)+" "+msg, lv});
    if (consoleLog_.size()>kMaxConsoleLines) consoleLog_.erase(consoleLog_.begin());
}

// ===================================================================
// Menu Bar & Toolbar
// ===================================================================
void Editor::DrawMainMenuBar() {
    if (!ImGui::BeginMainMenuBar()) return;
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
            World* w=externalWorld_?externalWorld_:&editorWorld_;
            if(w) w->Clear(); selectedObjectIndex_=-1; Log("New scene");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) SaveScene("assets/scenes/default.scene");
        if (ImGui::MenuItem("Save Scene As...")) SaveScene("assets/scenes/scene_"+std::to_string(std::time(nullptr))+".scene");
        if (ImGui::MenuItem("Load Scene...", "Ctrl+O")) LoadScene("assets/scenes/default.scene");
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Alt+F4")) glfwSetWindowShouldClose(window_,GLFW_TRUE);
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
        if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
        ImGui::Separator();
        if (ImGui::MenuItem("Delete", "Del")) DeleteSelectedActor();
        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) DuplicateSelectedActor();
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Project",nullptr,nullptr,true);
        ImGui::MenuItem("Hierarchy",nullptr,nullptr,true);
        ImGui::MenuItem("Inspector",nullptr,nullptr,true);
        ImGui::MenuItem("Viewport",nullptr,nullptr,true);
        ImGui::MenuItem("Content Browser",nullptr,nullptr,true);
        ImGui::MenuItem("Console",nullptr,nullptr,true);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void Editor::DrawToolbar() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4,3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(8,3));
    ImGui::Begin("##Toolbar",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoScrollbar|
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoMove);
    ImVec2 mp=ImGui::GetMainViewport()->Pos;
    ImGui::SetWindowPos(ImVec2(mp.x,mp.y+ImGui::GetFrameHeight()));
    ImGui::SetWindowSize(ImVec2(ImGui::GetMainViewport()->Size.x,0));
    float bh=ImGui::GetFrameHeight();
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    // Play controls
    const char* playLabel = playing_ ? "Stop" : "Play";
    ImVec4 playCol = playing_ ? ImVec4(0.9f,0.3f,0.2f,1) : ImVec4(0.2f,0.8f,0.3f,1);
    ImGui::PushStyleColor(ImGuiCol_Button, playCol);
    if (ImGui::Button(playLabel, ImVec2(0, bh))) {
        if (playing_) {
            // Stop
            playing_ = false; paused_ = false;
            runtimeWorld_.Clear();
            Log("Stopped");
        } else {
            // Play: copy editor world to runtime
            playing_ = true; paused_ = false;
            World* ew = externalWorld_ ? externalWorld_ : &editorWorld_;
            runtimeWorld_.Clear();
            for (auto& a : ew->actors) {
                auto* na = runtimeWorld_.SpawnActor(a->GetName());
                na->AddComponent<TransformComponent>("Transform");
                auto* st = a->GetComponent<TransformComponent>();
                auto* dt = na->GetComponent<TransformComponent>();
                if (st && dt) { dt->SetPosition(st->GetPosition()); dt->SetScale(st->GetScale()); }
            }
            Log("Playing (" + std::to_string(runtimeWorld_.actors.size()) + " actors)");
        }
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if(ImGui::Button("Save",ImVec2(0,bh))) SaveScene("assets/scenes/default.scene");
    ImGui::SameLine();
    if(ImGui::Button("Load",ImVec2(0,bh))) LoadScene("assets/scenes/default.scene");
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.45f,0.45f,0.45f,1),"%.0f FPS | %zu obj | %.2f ms",
        ImGui::GetIO().Framerate,w->actors.size(),1000.f/ImGui::GetIO().Framerate);
    ImGui::End(); ImGui::PopStyleVar(2);
}



// ===================================================================
// Viewport
// ===================================================================
void Editor::HandleViewportInput(const ImVec2& size, const ImVec2& pos) {
    ImGuiIO& io=ImGui::GetIO();
    bool hovered=ImGui::IsWindowHovered();
    if(hovered&&ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if(!vpCameraActive_) vpCameraActive_=true;
        vpCameraYaw_+=io.MouseDelta.x*0.3f; vpCameraPitch_-=io.MouseDelta.y*0.3f;
        vpCameraPitch_=glm::clamp(vpCameraPitch_,-89.f,89.f);
    }
    if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) vpCameraActive_=false;
    if(hovered&&ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        glm::vec3 fwd(cos(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)),
            sin(glm::radians(vpCameraPitch_)),sin(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)));
        glm::vec3 r=glm::normalize(glm::cross(fwd,glm::vec3(0,1,0)));
        glm::vec3 u=glm::cross(r,fwd);
        vpCameraPos_-=r*io.MouseDelta.x*0.01f; vpCameraPos_+=u*io.MouseDelta.y*0.01f;
    }
    if(hovered&&io.MouseWheel!=0) {
        glm::vec3 fwd(cos(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)),
            sin(glm::radians(vpCameraPitch_)),sin(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)));
        vpCameraPos_+=fwd*io.MouseWheel*0.5f;
    }
}

glm::mat4 Editor::ViewportViewMatrix() const {
    glm::vec3 fwd(cos(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)),
        sin(glm::radians(vpCameraPitch_)),sin(glm::radians(vpCameraYaw_))*cos(glm::radians(vpCameraPitch_)));
    return glm::lookAt(vpCameraPos_,vpCameraPos_+fwd,glm::vec3(0,1,0));
}

void Editor::DrawViewportPanel() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("Viewport",nullptr,ImGuiWindowFlags_NoScrollbar);
    ImVec2 vpPos=ImGui::GetWindowPos(), vpCPos=ImGui::GetCursorScreenPos();
    ImVec2 sz=ImGui::GetContentRegionAvail();
    HandleViewportInput(sz,vpPos);
    if(sz.x>0&&sz.y>0) {
        ImDrawList* dl=ImGui::GetWindowDrawList();
        dl->AddRectFilled(vpCPos,ImVec2(vpCPos.x+sz.x,vpCPos.y+sz.y),IM_COL32(18,18,20,255));
        float gs=64; ImU32 gc=IM_COL32(42,42,42,55), gcm=IM_COL32(54,54,54,85);
        for(float x=fmodf(-vpCPos.x,gs);x<sz.x;x+=gs) {
            float px=vpCPos.x+x; bool m=fmodf(fabsf(px),gs*5)<gs;
            dl->AddLine(ImVec2(px,vpCPos.y),ImVec2(px,vpCPos.y+sz.y),m?gcm:gc);
        }
        for(float y=fmodf(-vpCPos.y,gs);y<sz.y;y+=gs) {
            float py=vpCPos.y+y; bool m=fmodf(fabsf(py),gs*5)<gs;
            dl->AddLine(ImVec2(vpCPos.x,py),ImVec2(vpCPos.x+sz.x,py),m?gcm:gc);
        }
        // Axes
        float ax=vpCPos.x+32,ay=vpCPos.y+20;
        dl->AddLine(ImVec2(ax,ay),ImVec2(ax+25,ay),IM_COL32(220,60,60,200),2);
        dl->AddLine(ImVec2(ax,ay),ImVec2(ax,ay+25),IM_COL32(60,220,60,200),2);
        dl->AddText(ImVec2(ax+28,ay-4),IM_COL32(220,60,60,180),"X");
        dl->AddText(ImVec2(ax+4,ay+24),IM_COL32(60,220,60,180),"Y");
            DrawParticles(dl, vpCPos, sz);
            DrawParticles(dl, vpCPos, sz);
        // Overlay
        ImGui::SetCursorPos(ImVec2(8,sz.y-18));
        ImGui::TextColored(ImVec4(0.25f,0.25f,0.25f,0.35f),"%dx%d",(int)sz.x,(int)sz.y);
    }

    // Gizmo
    World* gw = externalWorld_ ? externalWorld_ : &editorWorld_;
    if (gw && selectedObjectIndex_ >= 0 && selectedObjectIndex_ < (int)gw->actors.size()) {
        auto* t = gw->actors[selectedObjectIndex_]->GetComponent<TransformComponent>();
        if (t) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(vpCPos.x, vpCPos.y, sz.x, sz.y);

            glm::mat4 view = ViewportViewMatrix();
            glm::mat4 proj = glm::perspective(glm::radians(60.0f), sz.x / sz.y, 0.1f, 1000.0f);
            proj[1][1] *= -1;

            glm::vec3 p = t->GetPosition();
            glm::vec3 s = t->GetScale();
            glm::mat4 model = glm::translate(glm::mat4(1), p) * glm::scale(glm::mat4(1), s);

            ImGuizmo::OPERATION op = (gizmoMode_ == 0) ? ImGuizmo::TRANSLATE :
                                     (gizmoMode_ == 1) ? ImGuizmo::ROTATE : ImGuizmo::SCALE;
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, ImGuizmo::LOCAL,
                                 glm::value_ptr(model));

            if (ImGuizmo::IsUsing()) {
                glm::vec3 newPos, newScl;
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model),
                    glm::value_ptr(newPos), nullptr, glm::value_ptr(newScl));
                t->SetPosition(newPos);
                if (gizmoMode_ == 2) t->SetScale(newScl);
            }
        }
    }

    ImGui::End(); ImGui::PopStyleVar();
}

void Editor::DrawStatsOverlay() {
    ImGuiWindowFlags fl=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|
        ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoDocking;
    ImVec2 mp=ImGui::GetMainViewport()->Pos;
    ImGui::SetNextWindowPos(ImVec2(mp.x+8,mp.y+50));
    ImGui::Begin("##SO",nullptr,fl);
    ImGui::TextColored(ImVec4(0.45f,0.7f,0.45f,0.85f),"%.0f FPS",ImGui::GetIO().Framerate);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f,0.35f,0.35f,0.6f)," %.2f ms",1000.f/ImGui::GetIO().Framerate);
    ImGui::End();
}

// ===================================================================
// Profiler Overlay
// ===================================================================
void Editor::DrawProfilerOverlay() {
    ImGuiWindowFlags fl = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;
    ImVec2 mp = ImGui::GetMainViewport()->Pos;
    ImGui::SetNextWindowPos(ImVec2(mp.x + 8, mp.y + 68));
    ImGui::Begin("##Profiler", nullptr, fl);
    static float times[4] = {0,0,0,0}; // update, culling, render, post
    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    if (dt > 0.25f) { // Update every 250ms
        times[0] = times[1]; times[1] = times[2]; times[2] = times[3]; times[3] = dt * 1000.0f;
        last = now;
    }
    const char* labels[] = {"Update","Cull","Render","Post"};
    ImU32 cols[] = {IM_COL32(80,140,200,180), IM_COL32(180,140,60,180), IM_COL32(200,80,80,180), IM_COL32(140,80,200,180)};
    for (int i = 0; i < 4; i++) {
        float w = std::max(1.0f, times[i]);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, cols[i]);
        ImGui::ProgressBar(times[i] / 5.0f, ImVec2(w, 10), labels[i]);
        ImGui::PopStyleColor();
        if (i < 3) ImGui::SameLine(0, 4);
    }
    ImGui::End();
}
// Particle Rendering (ImDrawList overlay in viewport)
// ===================================================================
void Editor::DrawParticles(ImDrawList* dl, const ImVec2& vpPos, const ImVec2& vpSize) {
    glm::mat4 view = ViewportViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), vpSize.x / vpSize.y, 0.1f, 1000.0f);
    proj[1][1] *= -1;
    glm::mat4 vp = proj * view;

    for (auto& p : particleSystem_.GetParticles()) {
        if (!p.alive) continue;
        glm::vec4 clip = vp * glm::vec4(p.position, 1.0f);
        if (clip.w <= 0.01f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.x < -1 || ndc.x > 1 || ndc.y < -1 || ndc.y > 1) continue;
        float sx = (ndc.x * 0.5f + 0.5f) * vpSize.x + vpPos.x;
        float sy = (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y + vpPos.y;
        float r = p.size * 300.0f / clip.w;
        r = glm::clamp(r, 1.0f, 20.0f);
        ImU32 col = IM_COL32(
            (int)(p.color.x*255), (int)(p.color.y*255),
            (int)(p.color.z*255), (int)(p.color.w*255));
        dl->AddCircleFilled(ImVec2(sx, sy), r, col);
    }
}

// ===================================================================
// ===================================================================
// Particle Panel
// ===================================================================
// ===================================================================
// Material Editor Panel
// ===================================================================
void Editor::DrawMaterialEditorPanel() {
    ImGui::Begin("Material Editor");
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Grid background
    dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+canvasSize.y), IM_COL32(28,28,32,255));
    float gs = 32;
    for (float x = fmodf(matScroll_.x, gs); x < canvasSize.x; x += gs)
        dl->AddLine(ImVec2(canvasPos.x+x, canvasPos.y), ImVec2(canvasPos.x+x, canvasPos.y+canvasSize.y), IM_COL32(40,40,44,120));
    for (float y = fmodf(matScroll_.y, gs); y < canvasSize.y; y += gs)
        dl->AddLine(ImVec2(canvasPos.x, canvasPos.y+y), ImVec2(canvasPos.x+canvasSize.x, canvasPos.y+y), IM_COL32(40,40,44,120));

    ImGui::InvisibleButton("##matcanvas", canvasSize);
    bool canvasHovered = ImGui::IsItemHovered();

    ImVec2 mouse = ImGui::GetMousePos();
    ImVec2 canvasMouse = ImVec2(mouse.x - canvasPos.x - matScroll_.x, mouse.y - canvasPos.y - matScroll_.y);

    // Pan with right click drag
    if (canvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        matScroll_.x += ImGui::GetIO().MouseDelta.x;
        matScroll_.y += ImGui::GetIO().MouseDelta.y;
    }

    // Right-click context: add node
    if (canvasHovered && ImGui::BeginPopupContextWindow("##matctx")) {
        if (ImGui::MenuItem("Constant (Color)")) materialGraph_.AddNode(NodeType::Constant4, glm::vec2(canvasMouse.x, canvasMouse.y));
        if (ImGui::MenuItem("Add")) materialGraph_.AddNode(NodeType::Add, glm::vec2(canvasMouse.x, canvasMouse.y));
        if (ImGui::MenuItem("Multiply")) materialGraph_.AddNode(NodeType::Multiply, glm::vec2(canvasMouse.x, canvasMouse.y));
        if (ImGui::MenuItem("Lerp")) materialGraph_.AddNode(NodeType::Lerp, glm::vec2(canvasMouse.x, canvasMouse.y));
        ImGui::EndPopup();
    }

    // Compile button
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    if (ImGui::Button("Compile")) {
        std::string glsl = materialGraph_.CompileToGLSL();
        Log("Material compiled:\n" + glsl);
    }

    // Draw connections
    for (auto& link : materialGraph_.GetLinks()) {
        // Find pin positions
        ImVec2 from(0,0), to(0,0);
        for (auto& n : materialGraph_.GetNodes()) {
            for (auto& p : n.outputs) if (p.id == link.fromPin) {
                from = ImVec2(canvasPos.x+n.position.x+matScroll_.x+140, canvasPos.y+n.position.y+matScroll_.y+30+12*(p.id+1));
            }
            for (auto& p : n.inputs) if (p.id == link.toPin) {
                to = ImVec2(canvasPos.x+n.position.x+matScroll_.x+10, canvasPos.y+n.position.y+matScroll_.y+30+12*(p.id+1));
            }
        }
        dl->AddBezierCubic(from, ImVec2(from.x+50,from.y), ImVec2(to.x-50,to.y), to, IM_COL32(255,220,100,200), 2.0f);
    }

    // Draw nodes
    for (auto& node : materialGraph_.GetNodes()) {
        ImVec2 np(canvasPos.x + node.position.x + matScroll_.x, canvasPos.y + node.position.y + matScroll_.y);
        ImVec2 ns(140, 20 + 12.0f * (std::max(node.inputs.size(), node.outputs.size())));
        ImU32 bg = IM_COL32(60,60,70,240);
        if (node.type == NodeType::Output) bg = IM_COL32(50,80,50,240);
        dl->AddRectFilled(np, ImVec2(np.x+ns.x, np.y+ns.y), bg, 4);
        dl->AddRect(np, ImVec2(np.x+ns.x, np.y+ns.y), IM_COL32(100,100,120,255), 4);
        dl->AddText(ImVec2(np.x+6, np.y+2), IM_COL32(255,255,255,200), node.name.c_str());

        // Input pins
        for (int i = 0; i < (int)node.inputs.size(); i++) {
            ImVec2 pp(np.x - 4, np.y + 22 + i*12);
            dl->AddCircleFilled(pp, 4, IM_COL32(255,200,100,255));
            dl->AddText(ImVec2(np.x+6, np.y+20+i*12), IM_COL32(180,180,180,200), node.inputs[i].name.c_str());
        }
        // Output pins
        for (int i = 0; i < (int)node.outputs.size(); i++) {
            ImVec2 pp(np.x + ns.x + 4, np.y + 22 + i*12);
            dl->AddCircleFilled(pp, 4, IM_COL32(255,200,100,255));
            dl->AddText(ImVec2(np.x+ns.x-50, np.y+20+i*12), IM_COL32(180,180,180,200), node.outputs[i].name.c_str());
        }

        // Drag node
        if (canvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (mouse.x >= np.x && mouse.x <= np.x+ns.x && mouse.y >= np.y && mouse.y <= np.y+ns.y) {
                matDragNode_ = node.id;
            }
        }
    }

    if (matDragNode_ >= 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        for (auto& n : materialGraph_.GetNodes()) {
            if (n.id == matDragNode_) {
                n.position.x += ImGui::GetIO().MouseDelta.x;
                n.position.y += ImGui::GetIO().MouseDelta.y;
            }
        }
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) matDragNode_ = -1;

    ImGui::End();
}

void Editor::DrawParticlePanel() {
    ImGui::Begin("Particles");
    static char emitName[64] = "new_emitter";
    if (ImGui::Button("Add Emitter", ImVec2(-1, 0))) {
        ParticleEmitter pe;
        pe.name = emitName;
        pe.spawnRate = 50.0f;
        pe.lifetime = 2.0f;
        pe.startSize = 0.06f;
        pe.endSize = 0.01f;
        pe.startColor = glm::vec4(1,1,1,1);
        pe.endColor = glm::vec4(1,0.5f,0,0);
        pe.velocity = glm::vec3(0, 0.5f, 0);
        pe.velocityRandom = 1.0f;
        pe.gravity = glm::vec3(0, -0.2f, 0);
        pe.maxParticles = 3000;
        particleSystem_.AddEmitter(pe);
        Log("Emitter added: " + pe.name);
    }
    ImGui::InputText("##emitname", emitName, 64);
    ImGui::Separator();
    auto& emitters = particleSystem_.GetEmitters();
    for (int i = 0; i < (int)emitters.size(); i++) {
        auto& e = emitters[i];
        bool en = e.enabled;
        ImGui::Checkbox(("##en" + std::to_string(i)).c_str(), &en);
        e.enabled = en;
        ImGui::SameLine();
        ImGui::Text("%s [%.0f/s]", e.name.c_str(), e.spawnRate);
        ImGui::SameLine(ImGui::GetWindowWidth() - 60);
        if (ImGui::SmallButton(("X##" + std::to_string(i)).c_str())) {
            particleSystem_.RemoveEmitter(e.name);
            break;
        }
    }
    int alive = 0;
    for (auto& p : particleSystem_.GetParticles()) if (p.alive) alive++;
    ImGui::Text("Alive: %d / Total: %d", alive, (int)particleSystem_.GetParticles().size());
    ImGui::End();
}

void Editor::CheckShaderChanges() {
    watcherTimer_ -= 0.016f;
    if (watcherTimer_ > 0) return;
    watcherTimer_ = 2.0f;
    try {
        for (auto& entry : std::filesystem::directory_iterator("shaders")) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".vert" && ext != ".frag" && ext != ".comp") continue;
            auto ft = std::filesystem::last_write_time(entry);
            std::string key = entry.path().string();
            if (shaderTimestamps_.count(key) && shaderTimestamps_[key] != ft)
                Log("Shader changed: " + key);
            shaderTimestamps_[key] = ft;
        }
    } catch (...) {}
}

// Particle Rendering (ImDrawList overlay in viewport)
// ===================================================================
// ===================================================================
// Project Panel
// ===================================================================
void Editor::RefreshProjectTree() {
    projectTree_.clear();
    try {
        for(auto& e:std::filesystem::recursive_directory_iterator("assets")) {
            std::string p=e.path().generic_string();
            if(e.is_directory()) p+="/";
            projectTree_.push_back(p);
        }
    } catch(...) {}
    std::sort(projectTree_.begin(),projectTree_.end());
}

void Editor::DrawProjectPanel() {
    ImGui::Begin("Project");
    if(ImGui::Button("Refresh")) { RefreshProjectTree(); RefreshAssetEntries(); }
    ImGui::SameLine();
    ImGui::Text("assets/");
    ImGui::Separator();
    for(auto& entry:projectTree_) {
        bool isDir=!entry.empty()&&entry.back()=='/';
        ImGuiTreeNodeFlags fl=ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_SpanAvailWidth;
        if(entry==currentAssetPath_) fl|=ImGuiTreeNodeFlags_Selected;
        ImGui::TreeNodeEx(entry.c_str(),fl,"%s",entry.c_str());
        if(ImGui::IsItemClicked()) {
            if(isDir) { currentAssetPath_=entry; RefreshAssetEntries(); }
            else {
                std::string l=entry; std::transform(l.begin(),l.end(),l.begin(),::tolower);
                bool md=false;
                for(auto& e:kModelExts) if(EndsWith(l,e)) { md=true; break; }
                if(md&&onAssetActivated_) onAssetActivated_(entry);
            }
        }
    }
    ImGui::End();
}

// ===================================================================
// Content Browser
// ===================================================================
void Editor::RefreshAssetEntries() {
    assetEntries_.clear();
    std::set<std::string> seen;
    try {
        for(auto& e:std::filesystem::directory_iterator(currentAssetPath_)) {
            std::string n=e.path().filename().string();
            if(e.is_directory()) n+="/";
            // Filter
            if(assetFilter_==1) { // Models only
                std::string l=n; std::transform(l.begin(),l.end(),l.begin(),::tolower);
                bool md=false; for(auto& x:kModelExts) if(EndsWith(l,x)){md=true;break;}
                if(!md&&!e.is_directory()) continue;
            } else if(assetFilter_==2) { // Textures only
                std::string l=n; std::transform(l.begin(),l.end(),l.begin(),::tolower);
                bool tx=false; for(auto& x:kTexExts) if(EndsWith(l,x)){tx=true;break;}
                if(!tx&&!e.is_directory()) continue;
            }
            if(seen.insert(n).second) assetEntries_.push_back(n);
        }
    } catch(...) { assetEntries_.push_back("(cannot read)"); }
    std::sort(assetEntries_.begin(),assetEntries_.end());
}

void Editor::DrawContentBrowser() {
    ImGui::Begin("Content Browser");
    // Toolbar
    if(ImGui::Button("Refresh")) RefreshAssetEntries(); ImGui::SameLine();
    // Breadcrumbs
    std::string pathCopy=currentAssetPath_;
    if(pathCopy.size()>1&&pathCopy.back()=='/') pathCopy.pop_back();
    std::vector<std::string> parts;
    size_t pos=0; while((pos=pathCopy.find('/'))!=std::string::npos) {
        if(pos>0) parts.push_back(pathCopy.substr(0,pos+1));
        else if(!pathCopy.empty()) parts.push_back(pathCopy.substr(0,1));
        pathCopy=pathCopy.substr(pos+1);
        if(pos==0 && pathCopy.find('/')==0) break; // prevent infinite loop on leading /
    }
    if(!pathCopy.empty()) parts.push_back(pathCopy+"/");
    for(size_t i=0;i<parts.size();++i) {
        if(i>0) ImGui::SameLine(0,0);
        if(ImGui::SmallButton(parts[i].c_str())) {
            currentAssetPath_="";
            for(size_t j=0;j<=i;++j) currentAssetPath_+=parts[j];
            RefreshAssetEntries();
        }
        if(i<parts.size()-1) { ImGui::SameLine(0,0); ImGui::Text("/"); ImGui::SameLine(0,0); }
    }
    // Filter buttons
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x-200);
    if(ImGui::Button("All")){assetFilter_=0;RefreshAssetEntries();} ImGui::SameLine();
    if(ImGui::Button("Models")){assetFilter_=1;RefreshAssetEntries();} ImGui::SameLine();
    if(ImGui::Button("Textures")){assetFilter_=2;RefreshAssetEntries();}
    ImGui::Separator();

    float cs=80, pw=ImGui::GetContentRegionAvail().x;
    int cols=std::max(1,(int)(pw/(cs+8)));
    if(cols>1) ImGui::Columns(cols,nullptr,false);
    for(size_t i=0;i<assetEntries_.size();++i) {
        auto& e=assetEntries_[i]; ImGui::PushID((int)i);
        bool isDir=!e.empty()&&e.back()=='/';
        ImVec4 col=isDir?ImVec4(0.7f,0.7f,0.35f,1):ImVec4(0.6f,0.6f,0.6f,1);
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Button,isDir?ImVec4(0.22f,0.20f,0.08f,0.6f):ImVec4(0.09f,0.09f,0.09f,0.6f));
        std::string lb=isDir?e:e.substr(0,e.find_last_of('.'));
        if(lb.length()>10) lb=lb.substr(0,9)+"..";
        ImGui::Button(lb.c_str(),ImVec2(cs,cs));
        ImGui::PopStyleColor();
        ImGui::TextColored(col,"%s",lb.c_str()); ImGui::EndGroup();
        if(ImGui::IsItemHovered()&&ImGui::IsMouseDoubleClicked(0)) {
            if(isDir) { currentAssetPath_+=e; RefreshAssetEntries(); }
            else {
                std::string l=e; std::transform(l.begin(),l.end(),l.begin(),::tolower);
                bool md=false; for(auto& x:kModelExts) if(EndsWith(l,x)){md=true;break;}
                if(md&&onAssetActivated_) onAssetActivated_(currentAssetPath_+e);
                else if(md) Log("Loader not connected: "+e);
                else Log("Not a model: "+e);
            }
        }
        // Drag-drop disabled for stability
        ImGui::PopID(); if(cols>1) ImGui::NextColumn();
    }
    if(cols>1) ImGui::Columns(1);
    ImGui::End();
}

// ===================================================================
// Hierarchy
// ===================================================================
void Editor::DeleteSelectedActor() {
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w||selectedObjectIndex_<0||selectedObjectIndex_>=(int)w->actors.size()) return;
    auto& a=w->actors[selectedObjectIndex_];
    Log("Deleted: "+a->GetName());
    w->actors.erase(w->actors.begin()+selectedObjectIndex_);
    if(selectedObjectIndex_>=(int)w->actors.size()) selectedObjectIndex_=(int)w->actors.size()-1;
}

void Editor::DuplicateSelectedActor() {
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w||selectedObjectIndex_<0||selectedObjectIndex_>=(int)w->actors.size()) return;
    // For now just spawn empty — full duplicate needs deep copy
    auto* a=w->SpawnActor(w->actors[selectedObjectIndex_]->GetName()+"_Copy");
    a->AddComponent<TransformComponent>("Transform");
    Log("Duplicated: "+a->GetName());
}

void Editor::DrawHierarchyPanel() {
    ImGui::Begin("Hierarchy");
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w){ImGui::End(); return;}

    if(ImGui::Button("+ Add")){ w->SpawnActor("Actor_"+std::to_string(w->actors.size())); Log("Added actor");}
    ImGui::SameLine();
    if(ImGui::Button("Clear")){ w->Clear(); selectedObjectIndex_=-1; Log("Cleared");}
    ImGui::Separator();

    // Delete key shortcut
    if(ImGui::IsWindowFocused()&&ImGui::IsKeyPressed(ImGuiKey_Delete)) DeleteSelectedActor();

    for(int i=0;i<(int)w->actors.size();++i) {
        auto& a=w->actors[i]; bool sel=(i==selectedObjectIndex_);
        ImGuiTreeNodeFlags fl=ImGuiTreeNodeFlags_Leaf|ImGuiTreeNodeFlags_NoTreePushOnOpen|ImGuiTreeNodeFlags_SpanAvailWidth;
        if(sel) fl|=ImGuiTreeNodeFlags_Selected;

        // Rename mode
        if(renaming_&&sel) {
            ImGui::SetNextItemWidth(-1);
            if(ImGui::InputText("##rn",renameBuf_,sizeof(renameBuf_),ImGuiInputTextFlags_EnterReturnsTrue)) {
                a->SetName(renameBuf_); renaming_=false;
            }
            if(!ImGui::IsItemActive()&&!ImGui::IsItemHovered()) renaming_=false;
        } else {
            // Icon per component type
            const char* icon="[]";
            if(a->GetComponent<LightComponent>()) icon="(*)";
            else if(a->GetComponent<MeshComponent>()) icon="[M]";
            else if(a->GetComponent<eoa::CameraComponent>()) icon="[C]";
            ImGui::TreeNodeEx((void*)(intptr_t)i,fl,"%s %s",icon,a->GetName().c_str());
        }

        if(ImGui::IsItemClicked()) selectedObjectIndex_=i;

        // Right-click context menu
        std::string ctx="ActorCtx_"+std::to_string(i);
        if(ImGui::BeginPopupContextItem(ctx.c_str())) {
            selectedObjectIndex_=i;
            if(ImGui::MenuItem("Rename")){ strncpy(renameBuf_,a->GetName().c_str(),sizeof(renameBuf_)-1); renaming_=true; }
            if(ImGui::MenuItem("Duplicate","Ctrl+D")) DuplicateSelectedActor();
            ImGui::Separator();
            if(ImGui::MenuItem("Delete","Del")) { DeleteSelectedActor(); ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

// ===================================================================
// Inspector
// ===================================================================
void Editor::DrawInspectorCategory(const char* label, unsigned int color, std::function<void()> body) {
    ImGui::PushStyleColor(ImGuiCol_Header,color);
    float ca=(color>>16)&0xFF, cb=(color>>8)&0xFF, cc=color&0xFF; ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(ca/255.f,cb/255.f,cc/255.f,0.5f));
    if(ImGui::CollapsingHeader(label,ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PopStyleColor(2);
        ImGui::Indent(8); body(); ImGui::Unindent(8);
        ImGui::Spacing();
    } else { ImGui::PopStyleColor(2); }
}

void Editor::DrawInspectorPanel() {
    ImGui::Begin("Inspector");
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w||selectedObjectIndex_<0||selectedObjectIndex_>=(int)w->actors.size()) {
        ImGui::TextColored(ImVec4(0.4f,0.4f,0.4f,1),"Select an actor");
        ImGui::End(); return;
    }
    auto& a=w->actors[selectedObjectIndex_];

    // Name
    char nb[128]; strncpy(nb,a->GetName().c_str(),sizeof(nb)-1); nb[sizeof(nb)-1]=0;
    ImGui::SetNextItemWidth(-1);
    if(ImGui::InputText("##Name",nb,sizeof(nb))) a->SetName(nb);
    ImGui::Spacing();

    // Transform category
    auto* t=a->GetComponent<TransformComponent>();
    if(t) {
        DrawInspectorCategory("Transform",IM_COL32(80,140,200,120),[&](){
            glm::vec3 p=t->GetPosition();
            if(ImGui::DragFloat3("Position",glm::value_ptr(p),0.1f)) t->SetPosition(p);
            ImGui::SameLine(); if(ImGui::SmallButton("R##pos")) t->SetPosition(glm::vec3(0));
            glm::vec3 s=t->GetScale();
            if(ImGui::DragFloat3("Scale",glm::value_ptr(s),0.1f,0.01f,100)) t->SetScale(s);
            ImGui::SameLine(); if(ImGui::SmallButton("R##scl")) t->SetScale(glm::vec3(1));
        });
    }

    // Mesh category
    auto* m=a->GetComponent<MeshComponent>();
    if(m) {
        DrawInspectorCategory("Mesh",IM_COL32(180,140,60,120),[&](){
            ImGui::Text("Tris: %u",m->HasMesh()?m->GetMesh()->IndexCount()/3:0);
            auto mat=m->GetMaterial();
            if(mat) {
                ImGui::Text("Material: %s",mat->name.c_str());
                ImGui::ColorEdit3("Base Color",glm::value_ptr(mat->baseColor));
                ImGui::DragFloat("Roughness",&mat->roughness,0.01f,0,1);
                ImGui::DragFloat("Metallic",&mat->metallic,0.01f,0,1);
            }
        });
    }

    // Light category
    auto* l=a->GetComponent<LightComponent>();
    if(l) {
        DrawInspectorCategory("Light",IM_COL32(220,180,60,120),[&](){
            int lt=(int)l->GetLightType(); const char* tp[]={"Directional","Point","Spot"};
            if(ImGui::Combo("Type",&lt,tp,3)) l->SetLightType((LightType)lt);
            glm::vec3 c=l->GetColor();
            if(ImGui::ColorEdit3("Color",glm::value_ptr(c))) l->SetColor(c);
            float in=l->GetIntensity();
            if(ImGui::DragFloat("Intensity",&in,0.1f,0,100)) l->SetIntensity(in);
        });
    }

    ImGui::Separator();
    bool vis=a->IsVisible();
    if(ImGui::Checkbox("Visible",&vis)) a->SetVisible(vis);
    ImGui::End();
}

// ===================================================================
// Console
// ===================================================================
void Editor::DrawConsolePanel() {
    ImGui::Begin("Console");
    if(ImGui::Button("Clear")) consoleLog_.clear(); ImGui::SameLine();
    ImGui::Text("%zu lines",consoleLog_.size()); ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x-280);
    const char* flt[]={"All","Info","Warnings","Errors"};
    if(ImGui::Button(flt[consoleFilter_])) consoleFilter_=(consoleFilter_+1)%4;
    ImGui::SameLine(); ImGui::Checkbox("Auto-scroll",&consoleAutoScroll_);
    ImGui::Separator();

    ImGui::BeginChild("CS",ImVec2(0,0),false,ImGuiWindowFlags_HorizontalScrollbar);
    for(auto& e:consoleLog_) {
        if(consoleFilter_==1&&e.level!=0) continue;
        if(consoleFilter_==2&&e.level!=1) continue;
        if(consoleFilter_==3&&e.level!=2) continue;
        ImVec4 col=ImVec4(0.7f,0.7f,0.7f,1);
        if(e.level==2) col=ImVec4(1,0.3f,0.3f,1);
        else if(e.level==1) col=ImVec4(1,0.8f,0.2f,1);
        ImGui::TextColored(col,"%s",e.text.c_str());
    }
    if(consoleAutoScroll_&&ImGui::GetScrollY()>=ImGui::GetScrollMaxY()-4) ImGui::SetScrollHereY(1);
    ImGui::EndChild(); ImGui::End();
}

// ===================================================================
// Scene Save/Load
// ===================================================================
void Editor::SaveScene(const std::string& path) {
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w) return;
    std::filesystem::path fp(path);
    if(fp.has_parent_path()) std::filesystem::create_directories(fp.parent_path());
    std::ofstream f(path,std::ios::binary);
    if(!f){ Log("[ERROR] Cannot save: "+path); return; }
    size_t n=w->actors.size(); f.write((char*)&n,sizeof(n));
    for(auto& a:w->actors) {
        std::string nm=a->GetName(); size_t nl=nm.size();
        f.write((char*)&nl,sizeof(nl)); f.write(nm.c_str(),nl);
        auto* t=a->GetComponent<TransformComponent>();
        if(t){ glm::vec3 p=t->GetPosition(),s=t->GetScale();
            f.write((char*)&p,sizeof(p)); f.write((char*)&s,sizeof(s)); }
    }
    Log("Saved: "+path+" ("+std::to_string(n)+" actors)");
}

void Editor::LoadScene(const std::string& path) {
    World* w=externalWorld_?externalWorld_:&editorWorld_;
    if(!w) return;
    std::ifstream f(path,std::ios::binary);
    if(!f){ Log("[WARN] Not found: "+path); return; }
    w->Clear(); selectedObjectIndex_=-1;
    size_t n=0; f.read((char*)&n,sizeof(n));
    for(size_t i=0;i<n;++i) {
        size_t nl=0; f.read((char*)&nl,sizeof(nl));
        std::string nm(nl,0); f.read(&nm[0],nl);
        auto* a=w->SpawnActor(nm); a->AddComponent<TransformComponent>("Transform");
        auto* t=a->GetComponent<TransformComponent>();
        if(t){ glm::vec3 p,s; f.read((char*)&p,sizeof(p)); f.read((char*)&s,sizeof(s));
            if(!f) break; t->SetPosition(p); t->SetScale(s); }
    }
    Log("Loaded: "+path+" ("+std::to_string(n)+" actors)");
}

Editor::~Editor() {
    ImGui_ImplVulkan_Shutdown(); ImGui_ImplGlfw_Shutdown();
    if(imGuiDescriptorPool_!=VK_NULL_HANDLE) vkDestroyDescriptorPool(device_,imGuiDescriptorPool_,nullptr);
    ImGui::DestroyContext();
}

} // namespace eoa
