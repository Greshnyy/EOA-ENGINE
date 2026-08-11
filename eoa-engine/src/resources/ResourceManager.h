#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <typeinfo>

namespace eoa {

enum class ResourceType {
    Unknown = 0,
    Texture2D,
    Texture3D,
    TextureCube,
    Mesh,
    SkeletalMesh,
    Material,
    Shader,
    Sound,
    Font,
    Animation,
    Scene,
    Prefab,
    Script,
    Config
};

class IResource {
public:
    virtual ~IResource() = default;

    const std::string& GetName() const { return name_; }
    const std::string& GetPath() const { return path_; }
    ResourceType GetType() const { return type_; }
    bool IsLoaded() const { return loaded_; }
    size_t GetMemoryUsage() const { return memoryUsage_; }

    virtual bool Load(const std::string& path) = 0;
    virtual void Unload() { loaded_ = false; }

    using LoadCallback = std::function<void(IResource*)>;
    void AddLoadCallback(LoadCallback callback) {
        loadCallbacks_.push_back(std::move(callback));
    }

protected:
    std::string name_;
    std::string path_;
    ResourceType type_ = ResourceType::Unknown;
    bool loaded_ = false;
    size_t memoryUsage_ = 0;
    std::vector<LoadCallback> loadCallbacks_;

    void OnLoaded() {
        loaded_ = true;
        for (auto& cb : loadCallbacks_) {
            if (cb) {
                cb(this);
            }
        }
    }
};

using ResourcePtr = std::shared_ptr<IResource>;

class ResourceManager {
public:
    static ResourceManager& GetInstance() {
        static ResourceManager instance;
        return instance;
    }

    template<typename T>
    void RegisterType(ResourceType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        creators_[type] = []() -> IResource* { return new T(); };
        typeNames_[type] = typeid(T).name();
    }

    template<typename T>
    std::shared_ptr<T> Load(const std::string& path) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = cache_.find(path);
            if (it != cache_.end()) {
                return std::dynamic_pointer_cast<T>(it->second);
            }
        }

        auto resource = std::make_shared<T>();
        if (!resource->Load(path)) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto [it, inserted] = cache_.emplace(path, resource);
        return inserted ? resource : std::dynamic_pointer_cast<T>(it->second);
    }

    ResourcePtr LoadByPath(const std::string& path) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = cache_.find(path);
            if (it != cache_.end()) {
                return it->second;
            }
        }

        const ResourceType type = DetectResourceType(path);
        std::function<IResource*()> creator;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto creatorIt = creators_.find(type);
            if (creatorIt == creators_.end()) {
                return nullptr;
            }
            creator = creatorIt->second;
        }

        std::unique_ptr<IResource> resource(creator());
        if (!resource || !resource->Load(path)) {
            return nullptr;
        }

        ResourcePtr shared = std::move(resource);
        std::lock_guard<std::mutex> lock(mutex_);
        auto [it, inserted] = cache_.emplace(path, shared);
        return inserted ? shared : it->second;
    }

    void Unload(const std::string& path) {
        ResourcePtr resource;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = cache_.find(path);
            if (it == cache_.end()) {
                return;
            }
            resource = std::move(it->second);
            cache_.erase(it);
        }
        resource->Unload();
    }

    void UnloadAll() {
        std::vector<ResourcePtr> resources;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            resources.reserve(cache_.size());
            for (auto& [path, resource] : cache_) {
                resources.push_back(std::move(resource));
            }
            cache_.clear();
        }
        for (auto& resource : resources) {
            if (resource) {
                resource->Unload();
            }
        }
    }

    template<typename T>
    std::shared_ptr<T> Get(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    bool IsLoaded(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(path);
        return it != cache_.end() && it->second && it->second->IsLoaded();
    }

    size_t GetLoadedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    size_t GetTotalMemoryUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t total = 0;
        for (const auto& [path, resource] : cache_) {
            total += resource->GetMemoryUsage();
        }
        return total;
    }

    std::vector<ResourcePtr> FindByType(ResourceType type) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ResourcePtr> result;
        for (const auto& [path, resource] : cache_) {
            if (resource->GetType() == type) {
                result.push_back(resource);
            }
        }
        return result;
    }

    std::vector<ResourcePtr> FindByPrefix(const std::string& prefix) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<ResourcePtr> result;
        for (const auto& [path, resource] : cache_) {
            if (path.find(prefix) == 0) {
                result.push_back(resource);
            }
        }
        return result;
    }

private:
    ResourceManager() = default;

    static std::string NormalizeExtension(std::string extension) {
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension;
    }

    static ResourceType DetectResourceType(const std::string& path) {
        const auto slash = path.find_last_of("/\\");
        const auto dot = path.find_last_of('.');
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
            return ResourceType::Unknown;
        }

        const std::string extension = NormalizeExtension(path.substr(dot));
        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga") {
            return ResourceType::Texture2D;
        }
        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf" || extension == ".glb") {
            return ResourceType::Mesh;
        }
        if (extension == ".wav" || extension == ".ogg" || extension == ".mp3") {
            return ResourceType::Sound;
        }
        if (extension == ".mat") {
            return ResourceType::Material;
        }
        if (extension == ".shader" || extension == ".hlsl" || extension == ".glsl") {
            return ResourceType::Shader;
        }
        return ResourceType::Unknown;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, ResourcePtr> cache_;
    std::unordered_map<ResourceType, std::function<IResource*()>> creators_;
    std::unordered_map<ResourceType, std::string> typeNames_;
};

struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float tangentX, tangentY, tangentZ;
};

class Texture2D : public IResource {
public:
    Texture2D() { type_ = ResourceType::Texture2D; }

    bool Load(const std::string& path) override {
        path_ = path;
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        // TODO: Реальная загрузка текстуры (PNG/JPG/TGA) и GPU ресурса.
        loaded_ = true;
        memoryUsage_ = width_ * height_ * 4;
        OnLoaded();
        return true;
    }

    void Unload() override {
        data_.reset();
        loaded_ = false;
        memoryUsage_ = 0;
    }

    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetChannels() const { return channels_; }
    void* GetData() { return data_.get(); }
    void SetFilterLinear(bool linear) { filterLinear_ = linear; }
    void SetRepeat(bool repeat) { repeat_ = repeat; }

private:
    int width_ = 0;
    int height_ = 0;
    int channels_ = 4;
    std::unique_ptr<uint8_t[]> data_;
    bool filterLinear_ = true;
    bool repeat_ = false;
};

class Mesh : public IResource {
public:
    Mesh() { type_ = ResourceType::Mesh; }

    bool Load(const std::string& path) override {
        path_ = path;
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        // TODO: Загрузка меша (FBX, OBJ, glTF) и создание GPU буферов.
        loaded_ = true;
        OnLoaded();
        return true;
    }

    const std::vector<Vertex>& GetVertices() const { return vertices_; }
    const std::vector<uint32_t>& GetIndices() const { return indices_; }
    size_t GetVertexCount() const { return vertices_.size(); }
    size_t GetIndexCount() const { return indices_.size(); }

private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};

class Sound : public IResource {
public:
    Sound() { type_ = ResourceType::Sound; }

    bool Load(const std::string& path) override {
        path_ = path;
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        // TODO: Загрузка и декодирование WAV/OGG/MP3.
        loaded_ = true;
        OnLoaded();
        return true;
    }

    float GetDuration() const { return duration_; }
    int GetSampleRate() const { return sampleRate_; }
    int GetChannels() const { return audioChannels_; }

private:
    float duration_ = 0.0f;
    int sampleRate_ = 44100;
    int audioChannels_ = 2;
    std::vector<float> samples_;
};

} // namespace eoa
