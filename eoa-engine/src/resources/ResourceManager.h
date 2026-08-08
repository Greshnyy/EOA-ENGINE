#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>

namespace eoa {

// ============================================================================
// БАЗОВЫЙ КЛАСС РЕСУРСА
// ============================================================================

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
    
    // Переопредели для загрузки из файла
    virtual bool Load(const std::string& path) = 0;
    
    // Переопредели для выгрузки
    virtual void Unload() { loaded_ = false; }
    
    // События
    using LoadCallback = std::function<void(IResource*)>;
    void AddLoadCallback(LoadCallback callback) {
        loadCallbacks_.push_back(callback);
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
            cb(this);
        }
    }
};

// ============================================================================
// МЕНЕДЖЕР РЕСУРСОВ
// ============================================================================

using ResourcePtr = std::shared_ptr<IResource>;

class ResourceManager {
public:
    static ResourceManager& GetInstance() {
        static ResourceManager instance;
        return instance;
    }
    
    // Регистрация типа ресурса
    template<typename T>
    void RegisterType(ResourceType type) {
        std::lock_guard<std::mutex> lock(mutex_);
        creators_[type] = []() -> IResource* { return new T(); };
        typeNames_[type] = typeid(T).name();
    }
    
    // Загрузка ресурса
    template<typename T>
    std::shared_ptr<T> Load(const std::string& path) {
        // Проверка кэша
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = cache_.find(path);
            if (it != cache_.end()) {
                return std::static_pointer_cast<T>(it->second);
            }
        }
        
        // Создание и загрузка
        auto resource = std::make_shared<T>();
        if (resource->Load(path)) {
            std::lock_guard<std::mutex> lock(mutex_);
            cache_[path] = resource;
            
            // Отправка события
            // EventSystem::Send<ResourceLoadedEvent>(path, resource.get());
            
            return resource;
        }
        
        return nullptr;
    }
    
    // Загрузка без указания типа (по расширению)
    ResourcePtr LoadByPath(const std::string& path) {
        // Авто-определение типа по расширению
        ResourceType type = DetectResourceType(path);
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Проверка кэша
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            return it->second;
        }
        
        // Создание через фабрику
        auto creatorIt = creators_.find(type);
        if (creatorIt == creators_.end()) {
            return nullptr;
        }
        
        IResource* resource = creatorIt->second();
        resource->Load(path);
        
        auto shared = ResourcePtr(resource);
        cache_[path] = shared;
        
        return shared;
    }
    
    // Выгрузка ресурса
    void Unload(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            it->second->Unload();
            cache_.erase(it);
        }
    }
    
    // Выгрузка всех ресурсов
    void UnloadAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [path, resource] : cache_) {
            resource->Unload();
        }
        cache_.clear();
    }
    
    // Получение из кэша
    template<typename T>
    std::shared_ptr<T> Get(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(path);
        if (it != cache_.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    // Проверка наличия в кэше
    bool IsLoaded(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(path) != cache_.end();
    }
    
    // Статистика
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
    
    // Поиск ресурсов по типу
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
    
    // Поиск по префиксу пути
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
    
    ResourceType DetectResourceType(const std::string& path) {
        // Простая реализация по расширению
        if (path.find(".png") != std::string::npos || 
            path.find(".jpg") != std::string::npos ||
            path.find(".tga") != std::string::npos) {
            return ResourceType::Texture2D;
        }
        if (path.find(".fbx") != std::string::npos ||
            path.find(".obj") != std::string::npos ||
            path.find(".gltf") != std::string::npos ||
            path.find(".glb") != std::string::npos) {
            return ResourceType::Mesh;
        }
        if (path.find(".wav") != std::string::npos ||
            path.find(".ogg") != std::string::npos ||
            path.find(".mp3") != std::string::npos) {
            return ResourceType::Sound;
        }
        if (path.find(".mat") != std::string::npos) {
            return ResourceType::Material;
        }
        if (path.find(".shader") != std::string::npos ||
            path.find(".hlsl") != std::string::npos ||
            path.find(".glsl") != std::string::npos) {
            return ResourceType::Shader;
        }
        return ResourceType::Unknown;
    }
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ResourcePtr> cache_;
    std::unordered_map<ResourceType, std::function<IResource*()>> creators_;
    std::unordered_map<ResourceType, std::string> typeNames_;
};

// ============================================================================
// ПРИМЕР: ТЕКСТУРА
// ============================================================================

class Texture2D : public IResource {
public:
    Texture2D() { type_ = ResourceType::Texture2D; }
    
    bool Load(const std::string& path) override {
        path_ = path;
        // Извлечение имени из пути
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        
        // TODO: Реальная загрузка текстуры
        // - Загрузка файла (PNG, JPG, TGA)
        // - Создание GPU ресурса
        // - Настройка параметров (фильтрация, wrapping)
        
        loaded_ = true;
        memoryUsage_ = width_ * height_ * 4; // RGBA
        
        OnLoaded();
        return true;
    }
    
    void Unload() override {
        // TODO: Освобождение GPU памяти
        loaded_ = false;
        memoryUsage_ = 0;
    }
    
    // Геттеры
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetChannels() const { return channels_; }
    void* GetData() { return data_.get(); }
    
    // Настройки
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

// ============================================================================
// ПРИМЕР: MESH
// ============================================================================

struct Vertex {
    float x, y, z;      // Позиция
    float nx, ny, nz;   // Нормаль
    float u, v;         // UV координаты
    float tangentX, tangentY, tangentZ;
};

class Mesh : public IResource {
public:
    Mesh() { type_ = ResourceType::Mesh; }
    
    bool Load(const std::string& path) override {
        path_ = path;
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        
        // TODO: Загрузка меша (FBX, OBJ, glTF)
        // - Парсинг файла
        // - Построение вершин и индексов
        // - Создание GPU буферов
        
        loaded_ = true;
        
        OnLoaded();
        return true;
    }
    
    // Геттеры
    const std::vector<Vertex>& GetVertices() const { return vertices_; }
    const std::vector<uint32_t>& GetIndices() const { return indices_; }
    size_t GetVertexCount() const { return vertices_.size(); }
    size_t GetIndexCount() const { return indices_.size(); }
    
private:
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
};

// ============================================================================
// ПРИМЕР: ЗВУК
// ============================================================================

class Sound : public IResource {
public:
    Sound() { type_ = ResourceType::Sound; }
    
    bool Load(const std::string& path) override {
        path_ = path;
        auto pos = path.find_last_of("/\\");
        name_ = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        
        // TODO: Загрузка звука (WAV, OGG, MP3)
        // - Декодирование
        // - Создание аудио буфера
        
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
