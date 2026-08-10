#include "editor/editor.h"
#include "log.h"
#include "core/transform_component.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <initializer_list>

namespace eoa {
namespace {
void CheckVkResult(VkResult err) { if (err != VK_SUCCESS) EOA_FATAL("ImGui Vulkan error: VkResult=%d", err); }
bool HasExtension(const std::string& path, std::initializer_list<const char*> extensions) {
    const auto dot = path.find_last_of('.'); if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    for (const char* candidate : extensions) if (ext == candidate) return true;
    return false;
}
}

Editor::Editor(GLFWwindow* w, VkInstance inst, VkPhysicalDevice phys, VkDevice dev, uint32_t gfxFam, VkQueue gfxQ, VkRenderPass rp, uint32_t imgCnt) : window_(w), device_(dev) {
    InitImGui(inst, phys, dev, gfxFam, gfxQ, rp, imgCnt); RefreshAssetEntries(); RefreshProjectTree(); EOA_LOG("Editor initialized");
}

Editor::~Editor() {
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
    ImGui_ImplVulkan_Shutdown(); ImGui_ImplGlfw_Shutdown();
    if (ImGui::GetCurrentContext()) ImGui::DestroyContext();
    if (imGuiDescriptorPool_ != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device_, imGuiDescriptorPool_, nullptr); imGuiDescriptorPool_ = VK_NULL_HANDLE; }
}

void Editor::InitImGui(VkInstance inst, VkPhysicalDevice phys, VkDevice dev, uint32_t gfxFam, VkQueue gfxQ, VkRenderPass rp, uint32_t imgCnt) {
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO(); io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; io.IniFilename = nullptr;
    ImGui::StyleColorsDark(); ImGuiStyle& style = ImGui::GetStyle(); style.WindowRounding=3.0f; style.FrameRounding=2.0f; style.WindowPadding=ImVec2(5,5); style.FramePadding=ImVec2(6,3); style.ItemSpacing=ImVec2(5,4);
    ImGui_ImplGlfw_InitForVulkan(window_, true); CreateDescriptorPool();
    ImGui_ImplVulkan_InitInfo ii{}; ii.Instance=inst; ii.PhysicalDevice=phys; ii.Device=dev; ii.QueueFamily=gfxFam; ii.Queue=gfxQ; ii.PipelineCache=VK_NULL_HANDLE; ii.DescriptorPool=imGuiDescriptorPool_; ii.MinImageCount=imgCnt; ii.ImageCount=imgCnt; ii.Allocator=nullptr; ii.CheckVkResultFn=CheckVkResult; ii.PipelineInfoMain.RenderPass=rp; ii.PipelineInfoMain.Subpass=0; ii.PipelineInfoMain.MSAASamples=VK_SAMPLE_COUNT_1_BIT; ImGui_ImplVulkan_Init(&ii);
}

void Editor::CreateDescriptorPool() {
    VkDescriptorPoolSize sizes[]={{VK_DESCRIPTOR_TYPE_SAMPLER,1000},{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1000},{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,1000},{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,1000},{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1000},{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1000},{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,1000}};
    VkDescriptorPoolCreateInfo info{}; info.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; info.flags=VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; info.maxSets=7000; info.poolSizeCount=static_cast<uint32_t>(sizeof(sizes)/sizeof(sizes[0])); info.pPoolSizes=sizes; EOA_CHECK_VK(vkCreateDescriptorPool(device_,&info,nullptr,&imGuiDescriptorPool_));
}

void Editor::SetupDocking() {
    ImGuiID dock=ImGui::DockSpaceOverViewport(0,ImGui::GetMainViewport(),ImGuiDockNodeFlags_PassthruCentralNode); if(!firstFrame_) return; firstFrame_=false;
    ImGui::DockBuilderRemoveNode(dock); ImGui::DockBuilderAddNode(dock,ImGuiDockNodeFlags_DockSpace); ImGui::DockBuilderSetNodeSize(dock,ImGui::GetMainViewport()->Size);
    ImGuiID right=0,left=0,bottom=0; ImGui::DockBuilderSplitNode(dock,ImGuiDir_Right,0.24f,&right,&dock); ImGui::DockBuilderSplitNode(dock,ImGuiDir_Left,0.20f,&left,&dock); ImGui::DockBuilderSplitNode(dock,ImGuiDir_Down,0.24f,&bottom,&dock);
    ImGui::DockBuilderDockWindow("Project",left); ImGui::DockBuilderDockWindow("Hierarchy",left); ImGui::DockBuilderDockWindow("Inspector",right); ImGui::DockBuilderDockWindow("Content Browser",bottom); ImGui::DockBuilderDockWindow("Console",bottom); ImGui::DockBuilderDockWindow("Viewport",dock); ImGui::DockBuilderFinish(dock);
}

void Editor::NewFrame() {
    ImGui_ImplVulkan_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame(); SetupDocking();
    static auto previous=std::chrono::steady_clock::now(); const auto now=std::chrono::steady_clock::now(); const float dt=std::chrono::duration<float>(now-previous).count(); previous=now;
    if(!particleSystem_.GetEmitters().empty()) particleSystem_.Update(std::max(0.0f,dt),vpCameraPos_);
    DrawMainMenuBar(); DrawToolbar(); DrawProjectPanel(); DrawViewportPanel(); DrawHierarchyPanel(); DrawInspectorPanel(); DrawContentBrowser(); DrawConsolePanel(); DrawStatsOverlay(); DrawParticlePanel(); DrawMaterialEditorPanel(); DrawTextureViewer();
}
void Editor::Render(VkCommandBuffer cmd){ ImGui::Render(); ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),cmd); }
bool Editor::WantsCaptureMouse()const{return ImGui::GetIO().WantCaptureMouse;}
bool Editor::WantsCaptureKeyboard()const{return ImGui::GetIO().WantCaptureKeyboard;}

void Editor::Log(const std::string& msg){
    int level=0; if(msg.find("[ERROR]")!=std::string::npos||msg.find("[FATAL]")!=std::string::npos) level=2; else if(msg.find("[WARN]")!=std::string::npos) level=1;
    const std::time_t t=std::time(nullptr); std::tm local{};
#if defined(_WIN32)
    localtime_s(&local,&t);
#else
    localtime_r(&t,&local);
#endif
    char stamp[16]{}; std::strftime(stamp,sizeof(stamp),"%H:%M:%S",&local); consoleLog_.push_back({std::string(stamp)+" "+msg,level}); if(consoleLog_.size()>kMaxConsoleLines) consoleLog_.erase(consoleLog_.begin());
}

void Editor::DrawMainMenuBar(){
    if(!ImGui::BeginMainMenuBar()) return;
    if(ImGui::BeginMenu("File")){ if(ImGui::MenuItem("New Scene")){(externalWorld_?externalWorld_:&editorWorld_)->Clear(); selectedObjectIndex_=-1; Log("New scene");} if(ImGui::MenuItem("Save Scene")) SaveScene("assets/scenes/default.scene"); if(ImGui::MenuItem("Load Scene")) LoadScene("assets/scenes/default.scene"); ImGui::Separator(); if(ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(window_,GLFW_TRUE); ImGui::EndMenu(); }
    if(ImGui::BeginMenu("Edit")){ if(ImGui::MenuItem("Delete","Del")) DeleteSelectedActor(); if(ImGui::MenuItem("Duplicate","Ctrl+D")) DuplicateSelectedActor(); ImGui::EndMenu(); }
    if(ImGui::BeginMenu("View")){ ImGui::MenuItem("Viewport",nullptr,true); ImGui::MenuItem("Hierarchy",nullptr,true); ImGui::MenuItem("Inspector",nullptr,true); ImGui::MenuItem("Content Browser",nullptr,true); ImGui::MenuItem("Console",nullptr,true); ImGui::EndMenu(); }
    ImGui::EndMainMenuBar();
}

void Editor::DrawToolbar(){
    ImGui::Begin("##Toolbar",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoDocking);
    ImGui::PushStyleColor(ImGuiCol_Button,playing_?ImVec4(0.65f,0.18f,0.12f,1):ImVec4(0.16f,0.55f,0.22f,1)); if(ImGui::Button(playing_?"Stop":"Play")){playing_=!playing_; paused_=false; Log(playing_?"Play":"Stop");} ImGui::PopStyleColor(); ImGui::SameLine();
    if(ImGui::Button("Save")) SaveScene("assets/scenes/default.scene"); ImGui::SameLine(); if(ImGui::Button("Refresh")){RefreshAssetEntries();RefreshProjectTree();} ImGui::SameLine();
    World* world=externalWorld_?externalWorld_:&editorWorld_; ImGui::Text("%.0f FPS | %zu actors",ImGui::GetIO().Framerate,world->actors.size()); ImGui::End();
}

void Editor::HandleViewportInput(const ImVec2&,const ImVec2&){
    ImGuiIO& io=ImGui::GetIO(); if(ImGui::IsWindowHovered()&&ImGui::IsMouseDown(ImGuiMouseButton_Right)){vpCameraActive_=true;vpCameraYaw_+=io.MouseDelta.x*0.3f;vpCameraPitch_=glm::clamp(vpCameraPitch_-io.MouseDelta.y*0.3f,-89.0f,89.0f);} if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) vpCameraActive_=false;
    if(ImGui::IsWindowHovered()&&io.MouseWheel!=0.0f){float yaw=glm::radians(vpCameraYaw_),pitch=glm::radians(vpCameraPitch_);glm::vec3 f(std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch));vpCameraPos_+=f*io.MouseWheel*0.5f;}
}

glm::mat4 Editor::ViewportViewMatrix()const{float yaw=glm::radians(vpCameraYaw_),pitch=glm::radians(vpCameraPitch_);glm::vec3 f(std::cos(yaw)*std::cos(pitch),std::sin(pitch),std::sin(yaw)*std::cos(pitch));return glm::lookAt(vpCameraPos_,vpCameraPos_+f,glm::vec3(0,1,0));}

void Editor::DrawViewportPanel(){
    ImGui::Begin("Viewport",nullptr,ImGuiWindowFlags_NoScrollbar); ImVec2 pos=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail(); HandleViewportInput(size,pos); ImDrawList* dl=ImGui::GetWindowDrawList(); dl->AddRectFilled(pos,ImVec2(pos.x+size.x,pos.y+size.y),IM_COL32(18,20,22,255)); const float grid=64.0f;
    for(float x=std::fmod(-pos.x,grid);x<size.x;x+=grid) dl->AddLine(ImVec2(pos.x+x,pos.y),ImVec2(pos.x+x,pos.y+size.y),IM_COL32(48,48,52,120)); for(float y=std::fmod(-pos.y,grid);y<size.y;y+=grid) dl->AddLine(ImVec2(pos.x,pos.y+y),ImVec2(pos.x+size.x,pos.y+y),IM_COL32(48,48,52,120));
    DrawParticles(dl,pos,size); ImGui::SetCursorScreenPos(pos); ImGui::TextColored(ImVec4(0.65f,0.65f,0.65f,1),"Viewport | RMB orbit | Wheel zoom | W/E/R mode"); ImGui::End();
}

void Editor::DrawHierarchyPanel(){
    ImGui::Begin("Hierarchy"); World* world=externalWorld_?externalWorld_:&editorWorld_; if(ImGui::Button("+ Actor")) world->SpawnActor("Actor"); ImGui::Separator();
    for(size_t i=0;i<world->actors.size();++i){Actor* actor=world->actors[i].get();if(!actor)continue;if(ImGui::Selectable(actor->GetName().c_str(),selectedObjectIndex_==static_cast<int>(i)))selectedObjectIndex_=static_cast<int>(i);} ImGui::End();
}

void Editor::DrawInspectorPanel(){
    ImGui::Begin("Inspector"); World* world=externalWorld_?externalWorld_:&editorWorld_; if(selectedObjectIndex_>=0&&selectedObjectIndex_<static_cast<int>(world->actors.size())){Actor* actor=world->actors[selectedObjectIndex_].get();ImGui::Text("Actor: %s",actor->GetName().c_str());if(auto* t=actor->GetComponent<TransformComponent>()){glm::vec3 p=t->GetPosition(),s=t->GetScale();if(ImGui::DragFloat3("Position",glm::value_ptr(p),0.05f))t->SetPosition(p);if(ImGui::DragFloat3("Scale",glm::value_ptr(s),0.05f,0.001f,100.0f))t->SetScale(s);}}else ImGui::TextDisabled("No actor selected"); ImGui::End();
}

void Editor::DrawContentBrowser(){
    ImGui::Begin("Content Browser"); if(ImGui::Button("Refresh"))RefreshAssetEntries();ImGui::SameLine();ImGui::TextUnformatted(currentAssetPath_.c_str());ImGui::Separator();
    for(const auto& path:assetEntries_)if(ImGui::Selectable(path.c_str())){std::string full=currentAssetPath_+path;if(HasExtension(full,{".gltf",".glb",".obj",".fbx"})&&onAssetActivated_)onAssetActivated_(full);} ImGui::End();
}

void Editor::DrawConsolePanel(){ImGui::Begin("Console");if(ImGui::Button("Clear"))consoleLog_.clear();ImGui::SameLine();ImGui::Checkbox("Auto scroll",&consoleAutoScroll_);ImGui::Separator();for(const auto& e:consoleLog_)ImGui::TextUnformatted(e.text.c_str());ImGui::End();}
void Editor::DrawProjectPanel(){ImGui::Begin("Project");for(const auto& e:projectTree_)ImGui::BulletText("%s",e.c_str());ImGui::End();}
void Editor::DrawStatsOverlay(){ImGui::Begin("##Stats",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings);ImGui::Text("FPS %.1f",ImGui::GetIO().Framerate);ImGui::Text("Viewport %.0fx%.0f",ImGui::GetMainViewport()->Size.x,ImGui::GetMainViewport()->Size.y);ImGui::End();}
void Editor::DrawInspectorCategory(const char* label,unsigned int,std::function<void()> body){if(ImGui::CollapsingHeader(label,ImGuiTreeNodeFlags_DefaultOpen))body();}

void Editor::RefreshAssetEntries(){assetEntries_.clear();std::error_code ec;for(const auto& entry:std::filesystem::directory_iterator(currentAssetPath_,ec)){if(ec)break;assetEntries_.push_back(entry.path().filename().string());}std::sort(assetEntries_.begin(),assetEntries_.end());}
void Editor::RefreshProjectTree(){projectTree_={"assets/","assets/scenes/","assets/models/","assets/textures/","shaders/"};}

void Editor::SaveScene(const std::string& path){std::filesystem::path file(path);std::error_code ec;if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path(),ec);std::ofstream out(path);if(!out){Log("[ERROR] Failed to save scene: "+path);return;}World* world=externalWorld_?externalWorld_:&editorWorld_;out<<"EOA_SCENE 1\n";for(const auto& actor:world->actors){if(!actor)continue;out<<std::quoted(actor->GetName())<<'\n';if(auto* t=actor->GetComponent<TransformComponent>()){auto p=t->GetPosition(),s=t->GetScale();out<<p.x<<' '<<p.y<<' '<<p.z<<' '<<s.x<<' '<<s.y<<' '<<s.z<<'\n';}}Log("Saved scene: "+path);}
void Editor::LoadScene(const std::string& path){std::ifstream in(path);if(!in){Log("[WARN] Scene not found: "+path);return;}World* world=externalWorld_?externalWorld_:&editorWorld_;world->Clear();std::string header;std::getline(in,header);while(in){std::string name;if(!(in>>std::quoted(name)))break;float px,py,pz,sx,sy,sz;if(!(in>>px>>py>>pz>>sx>>sy>>sz))break;Actor* actor=world->SpawnActor(name);auto* t=actor->AddComponent<TransformComponent>("Transform");t->SetPosition(glm::vec3(px,py,pz));t->SetScale(glm::vec3(sx,sy,sz));}selectedObjectIndex_=-1;Log("Loaded scene: "+path);}
void Editor::DeleteSelectedActor(){World* world=externalWorld_?externalWorld_:&editorWorld_;if(selectedObjectIndex_<0||selectedObjectIndex_>=static_cast<int>(world->actors.size()))return;world->actors.erase(world->actors.begin()+selectedObjectIndex_);selectedObjectIndex_=-1;}
void Editor::DuplicateSelectedActor(){World* world=externalWorld_?externalWorld_:&editorWorld_;if(selectedObjectIndex_<0||selectedObjectIndex_>=static_cast<int>(world->actors.size()))return;Actor* source=world->actors[selectedObjectIndex_].get();Actor* copy=world->SpawnActor(source->GetName()+" Copy");auto* src=source->GetComponent<TransformComponent>();auto* dst=copy->AddComponent<TransformComponent>("Transform");if(src){dst->SetPosition(src->GetPosition()+glm::vec3(1,0,0));dst->SetScale(src->GetScale());}}

void Editor::DrawParticlePanel(){ImGui::Begin("Particles");ImGui::Text("Emitters: %zu",particleSystem_.GetEmitters().size());ImGui::End();}
void Editor::DrawMaterialEditorPanel(){ImGui::Begin("Material Editor");ImGui::TextDisabled("Material graph editor will be enabled after renderer graph API stabilization.");ImGui::End();}
void Editor::DrawParticles(ImDrawList* dl,const ImVec2& pos,const ImVec2& size){if(!dl||size.x<=0||size.y<=0||particleSystem_.GetEmitters().empty())return;float cx=pos.x+size.x*0.5f,cy=pos.y+size.y*0.5f;const auto& particles=particleSystem_.GetEmitters().front().particles;for(size_t i=0;i<std::min<size_t>(particles.size(),128);++i){const auto& p=particles[i];dl->AddCircleFilled(ImVec2(cx+p.position.x*25.0f,cy-p.position.y*25.0f),2.0f,IM_COL32(220,180,90,180));}}
void Editor::DrawProfilerOverlay(){}
void Editor::CheckShaderChanges(){}
void Editor::DrawTextureViewer(){}
std::string Editor::OpenFileDialog(const char*,const char*){return {};}
void Editor::ImportModel(){}
void Editor::ImportTexture(){}

} // namespace eoa
