#pragma once

#include "world/World.h"
#include "resources/ResourceManager.h"
#include <string>
#include <vector>

namespace eoa {

// ============================================================================
// УРОВЕНЬ (LEVEL)
// ============================================================================

class Level {
public:
    Level(const std::string& name = "Untitled") 
        : name_(name) {}
    
    virtual ~Level() = default;
    
    const std::string& GetName() const { return name_; }
    World* GetWorld() { return world_.get(); }
    const World* GetWorld() const { return world_.get(); }
    
    // Инициализация уровня
    virtual void OnBeginPlay() {}
    
    // Завершение уровня
    virtual void OnEndPlay() {}
    
    // Обновление уровня
    virtual void OnUpdate(double dt) {}
    
    // Фиксированное обновление (физика)
    virtual void OnFixedUpdate(double dt) {}
    
    // Сохранение в файл
    virtual bool Save(const std::string& path) {
        // TODO: Сериализация мира в JSON
        return true;
    }
    
    // Загрузка из файла
    virtual bool Load(const std::string& path) {
        // TODO: Десериализация мира из JSON
        name_ = ExtractNameFromPath(path);
        return true;
    }
    
    // Создание актора на уровне
    template<typename T = Actor>
    T* SpawnActor(const std::string& name = "") {
        if (!world_) {
            world_ = std::make_unique<World>();
        }
        return world_->CreateActor<T>(name);
    }
    
    // Уничтожение актора
    void DestroyActor(Actor* actor) {
        if (world_) {
            world_->DestroyActor(actor);
        }
    }

protected:
    std::string name_;
    std::unique_ptr<World> world_;
    
private:
    std::string ExtractNameFromPath(const std::string& path) {
        auto pos = path.find_last_of("/\\");
        std::string filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        
        // Удаление расширения
        auto extPos = filename.find_last_of('.');
        if (extPos != std::string::npos) {
            filename = filename.substr(0, extPos);
        }
        
        return filename;
    }
};

// ============================================================================
// МЕНЕДЖЕР УРОВНЕЙ
// ============================================================================

using LevelPtr = std::shared_ptr<Level>;

class LevelManager {
public:
    static LevelManager& GetInstance() {
        static LevelManager instance;
        return instance;
    }
    
    // Загрузка уровня
    bool LoadLevel(const std::string& levelPath) {
        EOA_LOG_INFO("Загрузка уровня: " + levelPath);
        
        // Выгрузка текущего уровня
        if (currentLevel_) {
            UnloadLevel();
        }
        
        // Создание и загрузка нового уровня
        currentLevel_ = std::make_shared<Level>();
        if (!currentLevel_->Load(levelPath)) {
            EOA_LOG_ERROR("Не удалось загрузить уровень: " + levelPath);
            currentLevel_.reset();
            return false;
        }
        
        // Отправка события
        gEvents.Send<LevelLoadedEvent>(levelPath);
        
        // Инициализация
        currentLevel_->OnBeginPlay();
        
        EOA_LOG_INFO("Уровень загружен: " + currentLevel_->GetName());
        return true;
    }
    
    // Выгрузка уровня
    void UnloadLevel() {
        if (currentLevel_) {
            std::string name = currentLevel_->GetName();
            currentLevel_->OnEndPlay();
            currentLevel_.reset();
            
            gEvents.Send<LevelUnloadedEvent>(name);
            
            EOA_LOG_INFO("Уровень выгружен: " + name);
        }
    }
    
    // Получение текущего уровня
    Level* GetCurrentLevel() {
        return currentLevel_.get();
    }
    
    const Level* GetCurrentLevel() const {
        return currentLevel_.get();
    }
    
    // Есть ли загруженный уровень
    bool HasLevel() const {
        return currentLevel_ != nullptr;
    }
    
    // Обновление уровня
    void UpdateLevel(double dt) {
        if (currentLevel_) {
            currentLevel_->OnUpdate(dt);
        }
    }
    
    void FixedUpdateLevel(double dt) {
        if (currentLevel_) {
            currentLevel_->OnFixedUpdate(dt);
        }
    }
    
    // Мировой мир (persistent)
    World* GetPersistentWorld() {
        return &persistentWorld_;
    }

private:
    LevelManager() = default;
    
    LevelPtr currentLevel_;
    World persistentWorld_;
};

} // namespace eoa
