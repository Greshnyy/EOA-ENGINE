#pragma once

#include "Resources/Resource.h"
#include "Core/Logger.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <future>
#include <functional>

namespace EOA {

// ============================================
// МЕНЕДЖЕР РЕСУРСОВ (в стиле UE: UAssetManager)
// ============================================

class ResourceManager {
public:
    static ResourceManager* GetInstance() {
        static ResourceManager instance;
        return &instance;
    }
    
    // Загрузка ресурса (синхронно)
    template<typename T, typename... Args>
    T* LoadResource(const std::string& path, Args&&... args) {
        // Проверка кэша
        auto it = ResourceCache.find(path);
        if (it != ResourceCache.end()) {
            return static_cast<T*>(it->second.get());
        }
        
        // Создание и загрузка
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        resource->Path = path;
        
        // Извлечение имени из пути
        size_t lastSlash = path.find_last_of("/\\");
        resource->Name = (lastSlash != std::string::npos) ? 
                         path.substr(lastSlash + 1) : path;
        
        T* ptr = resource.get();
        ResourceCache[path] = std::move(resource);
        
        EOA_LOG_INFO("Loaded resource: " + path);
        
        return ptr;
    }
    
    // Асинхронная загрузка
    template<typename T, typename... Args>
    std::future<T*> LoadResourceAsync(const std::string& path, Args&&... args) {
        return std::async(std::launch::async, [this, path, args...]() {
            return LoadResource<T>(path, std::forward<Args>(args)...);
        });
    }
    
    // Выгрузка ресурса
    void UnloadResource(const std::string& path) {
        auto it = ResourceCache.find(path);
        if (it != ResourceCache.end()) {
            EOA_LOG_INFO("Unloaded resource: " + path);
            ResourceCache.erase(it);
        }
    }
    
    // Выгрузка всех ресурсов
    void UnloadAll() {
        ResourceCache.clear();
        EOA_LOG_INFO("All resources unloaded");
    }
    
    // Получение ресурса из кэша
    template<typename T>
    T* GetResource(const std::string& path) {
        auto it = ResourceCache.find(path);
        if (it != ResourceCache.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }
    
    // Проверка наличия ресурса
    bool IsResourceLoaded(const std::string& path) const {
        return ResourceCache.find(path) != ResourceCache.end();
    }
    
    // Статистика
    size_t GetLoadedResourceCount() const { return ResourceCache.size(); }
    
    size_t GetTotalMemoryUsage() const {
        size_t total = 0;
        for (const auto& [path, resource] : ResourceCache) {
            if (auto gpuRes = dynamic_cast<const GPUResource*>(resource.get())) {
                total += gpuRes->GetMemoryUsage();
            }
        }
        return total;
    }
    
    // Установка базового пути
    void SetBasePath(const std::string& basePath) { BasePath = basePath; }
    const std::string& GetBasePath() const { return BasePath; }
    
    // Полный путь к ресурсу
    std::string GetFullPath(const std::string& relativePath) const {
        return BasePath + "/" + relativePath;
    }
    
private:
    ResourceManager() = default;
    
    std::unordered_map<std::string, std::unique_ptr<Resource>> ResourceCache;
    std::string BasePath = "assets";
};

#define gResources EOA::ResourceManager::GetInstance()

// Удобные макросы для загрузки
#define EOA_LOAD_TEXTURE(path) gResources->LoadResource<Texture>(gResources->GetFullPath(path))
#define EOA_LOAD_MESH(path) gResources->LoadResource<Mesh>(gResources->GetFullPath(path))
#define EOA_LOAD_SHADER(path) gResources->LoadResource<Shader>(gResources->GetFullPath(path))

} // namespace EOA
