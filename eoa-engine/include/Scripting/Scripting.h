#pragma once
#include "core/component.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

// Forward declare lua_State
typedef struct lua_State lua_State;

namespace eoa {

// ============================================================================
// SCRIPTING SYSTEM - Lua интеграция
// ============================================================================

class ScriptComponent;

// ============================================================================
// ScriptClass - регистрация C++ классов в Lua
// ============================================================================

class ScriptClass {
public:
    using ConstructorFunc = std::function<void*(lua_State*)>;
    using MethodFunc = std::function<int(lua_State*)>;
    using StaticMethodFunc = std::function<int(lua_State*)>;
    
    explicit ScriptClass(const std::string& name);
    ~ScriptClass();
    
    // Конструктор
    ScriptClass& AddConstructor(ConstructorFunc func);
    
    // Методы экземпляра
    ScriptClass& AddMethod(const std::string& name, MethodFunc func);
    
    // Статические методы
    ScriptClass& AddStaticMethod(const std::string& name, StaticMethodFunc func);
    
    // Свойства (getter/setter)
    ScriptClass& AddProperty(const std::string& name, 
                             MethodFunc getter, 
                             MethodFunc setter = nullptr);
    
    // Наследование
    ScriptClass& Extends(const std::string& parentName);
    
    // Регистрация в Lua
    void Register(lua_State* L);
    
    const std::string& GetName() const { return name_; }
    
private:
    std::string name_;
    std::string parentName_;
    ConstructorFunc constructor_;
    std::unordered_map<std::string, MethodFunc> methods_;
    std::unordered_map<std::string, StaticMethodFunc> staticMethods_;
    std::unordered_map<std::string, MethodFunc> getters_;
    std::unordered_map<std::string, MethodFunc> setters_;
};

// ============================================================================
// ScriptModule - модуль скрипта
// ============================================================================

class ScriptModule {
public:
    explicit ScriptModule(const std::string& name);
    ~ScriptModule();
    
    // Добавить функцию в модуль
    void AddFunction(const std::string& name, 
                     std::function<int(lua_State*)> func);
    
    // Регистрация в Lua
    void Register(lua_State* L);
    
private:
    std::string name_;
    std::unordered_map<std::string, std::function<int(lua_State*)>> functions_;
};

// ============================================================================
// ScriptComponent - компонент скрипта для Actor
// ============================================================================

class ScriptComponent : public Component {
public:
    EOA_CLASS_DECL(ScriptComponent, Component)
    
    explicit ScriptComponent(const std::string& name = "Script");
    ~ScriptComponent() override;
    
    // Загрузка скрипта
    bool LoadScript(const std::string& filename);
    void UnloadScript();
    
    // Вызов функций из скрипта
    template<typename... Args>
    void CallFunction(const std::string& funcName, Args&&... args) {
        if (!scriptLoaded_) return;
        callFunctionInternal(funcName, std::forward<Args>(args)...);
    }
    
    // Получение значений из скрипта
    template<typename T>
    T GetGlobal(const std::string& name) {
        return getGlobalInternal<T>(name);
    }
    
    // Установка значений в скрипт
    template<typename T>
    void SetGlobal(const std::string& name, T value) {
        setGlobalInternal(name, value);
    }
    
    // Lifecycle вызовы
    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;
    
    // Проверка наличия функции
    bool HasFunction(const std::string& funcName) const;
    
    // Callbacks для жизненного цикла
    using ScriptCallback = std::function<void()>;
    using ScriptTickCallback = std::function<void(float)>;
    
    void SetOnInit(ScriptCallback callback) { onInit_ = callback; }
    void SetOnTick(ScriptTickCallback callback) { onTick_ = callback; }
    void SetOnShutdown(ScriptCallback callback) { onShutdown_ = callback; }
    
    // Доступ к Lua state (для продвинутого использования)
    lua_State* GetLuaState() const { return luaState_; }
    
    // Путь к скрипту
    const std::string& GetScriptPath() const { return scriptPath_; }
    
    // Hot reload
    bool ReloadScript();
    
private:
    std::string scriptPath_;
    bool scriptLoaded_ = false;
    
    lua_State* luaState_ = nullptr;
    bool ownsLuaState_ = false;  // Если true, то мы создали state
    
    ScriptCallback onInit_;
    ScriptTickCallback onTick_;
    ScriptCallback onShutdown_;
    
    // Внутренние методы для работы с Lua
    void callFunctionInternal(const std::string& funcName);
    template<typename T, typename... Args>
    void callFunctionInternal(const std::string& funcName, T&& arg, Args&&... args);
    
    template<typename T>
    T getGlobalInternal(const std::string& name);
    
    template<typename T>
    void setGlobalInternal(const std::string& name, T value);
    
    // Push/pop значений в стек Lua
    void pushToStack(lua_State* L, int value);
    void pushToStack(lua_State* L, float value);
    void pushToStack(lua_State* L, double value);
    void pushToStack(lua_State* L, const std::string& value);
    void pushToStack(lua_State* L, bool value);
    void pushToStack(lua_State* L, glm::vec3 value);
    void pushToStack(lua_State* L, Actor* actor);
    
    int popFromStack(lua_State* L);
    float popFloatFromStack(lua_State* L);
    std::string popStringFromStack(lua_State* L);
    bool popBoolFromStack(lua_State* L);
    glm::vec3 popVec3FromStack(lua_State* L);
};

// ============================================================================
// ScriptManager - глобальный менеджер скриптов
// ============================================================================

class ScriptManager {
public:
    static ScriptManager& Get() {
        static ScriptManager instance;
        return instance;
    }
    
    // Инициализация/завершение
    bool Initialize(bool loadStandardLibs = true);
    void Shutdown();
    
    // Получить глобальный Lua state
    lua_State* GetGlobalState() const { return globalState_; }
    
    // Регистрация классов
    void RegisterClass(std::unique_ptr<ScriptClass> scriptClass);
    ScriptClass* FindClass(const std::string& name);
    
    // Регистрация модулей
    void RegisterModule(std::unique_ptr<ScriptModule> module);
    
    // Загрузка скрипта
    bool LoadScript(const std::string& filename, ScriptComponent* owner = nullptr);
    bool LoadScriptFromString(const std::string& code, const std::string& name = "inline");
    
    // Выгрузка скрипта
    void UnloadScript(const std::string& filename);
    void UnloadAllScripts();
    
    // Hot reload всех скриптов
    void ReloadAllScripts();
    
    // Выполнение кода
    bool ExecuteCode(const std::string& code);
    
    // Пути поиска скриптов
    void AddSearchPath(const std::string& path);
    const std::vector<std::string>& GetSearchPaths() const { return searchPaths_; }
    
    // Стандартные библиотеки
    void LoadStandardLibrary(const std::string& name);
    
    // Garbage collection
    void GarbageCollect();
    void SetGCThreshold(int threshold);
    
    // Статистика
    int GetLoadedScriptCount() const { return static_cast<int>(loadedScripts_.size()); }
    int GetRegisteredClassCount() const { return static_cast<int>(registeredClasses_.size()); }
    
private:
    ScriptManager() = default;
    ~ScriptManager();
    
    lua_State* globalState_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<ScriptClass>> registeredClasses_;
    std::unordered_map<std::string, std::unique_ptr<ScriptModule>> registeredModules_;
    std::unordered_map<std::string, std::string> loadedScripts_;  // filename -> code
    std::vector<std::string> searchPaths_;
    
    bool initialized_ = false;
    int gcThreshold_ = 800;
};

// ============================================================================
// Утилиты для работы с Lua
// ============================================================================

namespace ScriptUtils {
    // Проверка типа в Lua
    bool IsNil(lua_State* L, int index);
    bool IsNumber(lua_State* L, int index);
    bool IsString(lua_State* L, int index);
    bool IsTable(lua_State* L, int index);
    bool IsFunction(lua_State* L, int index);
    bool IsUserdata(lua_State* L, int index);
    
    // Безопасное получение значения
    int SafeGetInteger(lua_State* L, int index, int defaultValue = 0);
    float SafeGetNumber(lua_State* L, int index, float defaultValue = 0.0f);
    std::string SafeGetString(lua_State* L, int index, const std::string& defaultValue = "");
    bool SafeGetBool(lua_State* L, int index, bool defaultValue = false);
    
    // Ошибка в Lua
    std::string GetLastError(lua_State* L);
    void PrintStackTrace(lua_State* L);
    
    // Загрузить файл
    int LoadFile(lua_State* L, const std::string& filename);
}

// ============================================================================
// Макросы для регистрации классов
// ============================================================================

#define EOA_SCRIPT_CLASS(className) \
    static eoa::ScriptClass* RegisterScriptClass_##className() { \
        auto* scriptClass = new eoa::ScriptClass(#className); \
        RegisterInScriptClass(scriptClass); \
        eoa::ScriptManager::Get().RegisterClass(std::unique_ptr<eoa::ScriptClass>(scriptClass)); \
        return scriptClass; \
    } \
    static eoa::ScriptClass* scriptClassInstance_##className = RegisterScriptClass_##className(); \
    static void RegisterInScriptClass(eoa::ScriptClass* scriptClass)

#define EOA_SCRIPT_METHOD(className, methodName) \
    scriptClass->AddMethod(#methodName, [](lua_State* L) -> int { \
        auto* self = reinterpret_cast<className*>(lua_touserdata(L, 1)); \
        if (!self) { \
            lua_pushstring(L, "Invalid 'self' parameter"); \
            lua_error(L); \
            return 0; \
        } \
        return self->methodName(L); \
    })

#define EOA_SCRIPT_STATIC_METHOD(className, methodName) \
    scriptClass->AddStaticMethod(#methodName, [](lua_State* L) -> int { \
        return className::methodName(L); \
    })

} // namespace eoa
