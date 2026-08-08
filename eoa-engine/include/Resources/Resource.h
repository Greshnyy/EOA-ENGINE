#pragma once

#include "Core/Platform.h"
#include <string>
#include <atomic>

namespace EOA {

// Базовый класс для всех ресурсов
class EOA_API Resource {
public:
    virtual ~Resource() = default;
    
    // Имя ресурса
    const std::string& GetName() const { return Name; }
    void SetName(const std::string& name) { Name = name; }
    
    // Путь к файлу (если загружен из файла)
    const std::string& GetPath() const { return Path; }
    
    // Статус загрузки
    bool IsLoaded() const { return LoadStatus.load(); }
    
    // Reference counting для управления памятью
    void AddRef() { RefCount.fetch_add(1); }
    void Release() {
        if (RefCount.fetch_sub(1) == 1) {
            delete this;
        }
    }
    
protected:
    std::string Name;
    std::string Path;
    std::atomic<bool> LoadStatus{false};
    std::atomic<int> RefCount{1};
};

// Типы ресурсов
enum class ResourceType {
    Unknown,
    Texture2D,
    TextureCube,
    Mesh,
    Material,
    Shader,
    AudioClip,
    Font,
    Scene,
    Prefab,
    Script
};

} // namespace EOA
