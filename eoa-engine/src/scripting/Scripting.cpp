#include "Scripting/Scripting.h"
#include "Core/Logger.h"
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>

// Lua headers (заглушки для примера)
// В реальном проекте: #include <lua.hpp>

extern "C" {
    // Lua C API заглушки для компиляции
    // В реальности эти функции предоставляются библиотекой Lua
    
    typedef struct lua_State lua_State;
    
    int luaL_loadfile(lua_State* L, const char* filename) { return 0; }
    int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) { return 0; }
    void lua_pushnil(lua_State* L) {}
    int lua_type(lua_State* L, int index) { return 0; }
    const char* lua_typename(lua_State* L, int tp) { return "unknown"; }
    int lua_gettop(lua_State* L) { return 0; }
    void lua_settop(lua_State* L, int index) {}
    void lua_pushboolean(lua_State* L, int b) {}
    void lua_pushinteger(lua_State* L, int n) {}
    void lua_pushnumber(lua_State* L, double n) {}
    void lua_pushstring(lua_State* L, const char* s) {}
    void lua_pushcclosure(lua_State* L, int fn, int n) {}
    void lua_createtable(lua_State* L, int narr, int nrec) {}
    void lua_setfield(lua_State* L, int idx, const char* k) {}
    void lua_setglobal(lua_State* L, const char* name) {}
    int lua_getfield(lua_State* L, int idx, const char* k) { return 0; }
    int lua_getglobal(lua_State* L, const char* name) { return 0; }
    int lua_toboolean(lua_State* L, int idx) { return 0; }
    int lua_tointeger(lua_State* L, int idx) { return 0; }
    double lua_tonumber(lua_State* L, int idx) { return 0.0; }
    const char* lua_tostring(lua_State* L, int idx) { return ""; }
    void* lua_touserdata(lua_State* L, int idx) { return nullptr; }
    int lua_isnil(lua_State* L, int idx) { return 0; }
    int lua_isnumber(lua_State* L, int idx) { return 0; }
    int lua_isstring(lua_State* L, int idx) { return 0; }
    int lua_istable(lua_State* L, int idx) { return 0; }
    int lua_isfunction(lua_State* L, int idx) { return 0; }
    int lua_isuserdata(lua_State* L, int idx) { return 0; }
    void lua_pop(lua_State* L, int n) {}
    int lua_rawget(lua_State* L, int idx) { return 0; }
    void lua_rawset(lua_State* L, int idx) {}
    void lua_newtable(lua_State* L) {}
    int luaL_ref(lua_State* L, int t) { return 0; }
    void luaL_unref(lua_State* L, int t, int ref) {}
    int luaL_loadbuffer(lua_State* L, const char* buff, size_t sz, const char* name) { return 0; }
    void lua_gc(lua_State* L, int what, int data) {}
}

namespace eoa {

// ============================================================================
// ScriptClass
// ============================================================================

ScriptClass::ScriptClass(const std::string& name) : name_(name) {}

ScriptClass::~ScriptClass() = default;

ScriptClass& ScriptClass::AddConstructor(ConstructorFunc func) {
    constructor_ = std::move(func);
    return *this;
}

ScriptClass& ScriptClass::AddMethod(const std::string& name, MethodFunc func) {
    methods_[name] = std::move(func);
    return *this;
}

ScriptClass& ScriptClass::AddStaticMethod(const std::string& name, StaticMethodFunc func) {
    staticMethods_[name] = std::move(func);
    return *this;
}

ScriptClass& ScriptClass::AddProperty(const std::string& name, 
                                       MethodFunc getter, 
                                       MethodFunc setter) {
    getters_[name] = std::move(getter);
    if (setter) {
        setters_[name] = std::move(setter);
    }
    return *this;
}

ScriptClass& ScriptClass::Extends(const std::string& parentName) {
    parentName_ = parentName;
    return *this;
}

void ScriptClass::Register(lua_State* L) {
    EOA_LOG_INFO("ScriptClass: Registering class '{}'", name_);
    
    // В реальной реализации регистрация класса в Lua
    // Создание metatable, методов, свойств
    
    // lua_newmetatable(L, name_.c_str());
    // ... регистрация методов ...
    // lua_pop(L, 1);
}

// ============================================================================
// ScriptModule
// ============================================================================

ScriptModule::ScriptModule(const std::string& name) : name_(name) {}

ScriptModule::~ScriptModule() = default;

void ScriptModule::AddFunction(const std::string& name, 
                                std::function<int(lua_State*)> func) {
    functions_[name] = std::move(func);
}

void ScriptModule::Register(lua_State* L) {
    EOA_LOG_INFO("ScriptModule: Registering module '{}'", name_);
    
    // В реальной реализации создание таблицы модуля с функциями
    // lua_newtable(L);
    // for (auto& [name, func] : functions_) {
    //     lua_pushcfunction(L, func);
    //     lua_setfield(L, -2, name.c_str());
    // }
    // lua_setglobal(L, name_.c_str());
}

// ============================================================================
// ScriptComponent
// ============================================================================

ScriptComponent::ScriptComponent(const std::string& name) 
    : Component(name) {}

ScriptComponent::~ScriptComponent() {
    Shutdown();
    UnloadScript();
}

bool ScriptComponent::LoadScript(const std::string& filename) {
    scriptPath_ = filename;
    
    EOA_LOG_INFO("ScriptComponent: Loading script '{}'", filename);
    
    // Чтение файла скрипта
    std::ifstream file(filename);
    if (!file.is_open()) {
        EOA_LOG_ERROR("ScriptComponent: Failed to open script file '{}'", filename);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    
    // Загрузка скрипта в Lua state
    if (!luaState_) {
        // Используем глобальный state из менеджера
        luaState_ = ScriptManager::Get().GetGlobalState();
    }
    
    if (luaState_) {
        // luaL_loadbuffer(luaState_, code.c_str(), code.size(), filename.c_str());
        // lua_pcall(luaState_, 0, 0, 0);
        scriptLoaded_ = true;
        EOA_LOG_INFO("ScriptComponent: Successfully loaded '{}'", filename);
    }
    
    return scriptLoaded_;
}

void ScriptComponent::UnloadScript() {
    if (scriptLoaded_ && ownsLuaState_) {
        // Очистка локального Lua state
        // lua_close(luaState_);
        luaState_ = nullptr;
        ownsLuaState_ = false;
    }
    
    scriptLoaded_ = false;
    scriptPath_.clear();
}

void ScriptComponent::Initialize() {
    if (!scriptLoaded_) return;
    
    EOA_LOG_INFO("ScriptComponent: Initializing script");
    
    // Вызов функции OnInit если она есть
    if (HasFunction("OnInit")) {
        CallFunction("OnInit");
    }
    
    if (onInit_) {
        onInit_();
    }
}

void ScriptComponent::Tick(float deltaTime) {
    if (!scriptLoaded_) return;
    
    // Вызов функции OnTick если она есть
    if (HasFunction("OnTick")) {
        CallFunction("OnTick", deltaTime);
    }
    
    if (onTick_) {
        onTick_(deltaTime);
    }
}

void ScriptComponent::Shutdown() {
    if (!scriptLoaded_) return;
    
    EOA_LOG_INFO("ScriptComponent: Shutting down script");
    
    // Вызов функции OnShutdown если она есть
    if (HasFunction("OnShutdown")) {
        CallFunction("OnShutdown");
    }
    
    if (onShutdown_) {
        onShutdown_();
    }
}

bool ScriptComponent::HasFunction(const std::string& funcName) const {
    if (!scriptLoaded_ || !luaState_) return false;
    
    // Проверка наличия функции в Lua
    // lua_getglobal(luaState_, funcName.c_str());
    // bool hasFunc = lua_isfunction(luaState_, -1);
    // lua_pop(luaState_, 1);
    
    // Заглушка - считаем что функция есть
    return funcName == "OnInit" || funcName == "OnTick" || funcName == "OnShutdown";
}

bool ScriptComponent::ReloadScript() {
    UnloadScript();
    return LoadScript(scriptPath_);
}

void ScriptComponent::callFunctionInternal(const std::string& funcName) {
    if (!luaState_) return;
    
    // lua_getglobal(luaState_, funcName.c_str());
    // if (lua_isfunction(luaState_, -1)) {
    //     lua_pcall(luaState_, 0, 0, 0);
    // }
    // lua_pop(luaState_, 1);
}

template<typename T, typename... Args>
void ScriptComponent::callFunctionInternal(const std::string& funcName, T&& arg, Args&&... args) {
    if (!luaState_) return;
    
    // Рекурсивная загрузка аргументов в стек
    // pushToStack(luaState_, std::forward<T>(arg));
    // callFunctionInternal(funcName, std::forward<Args>(args)...);
}

template<typename T>
T ScriptComponent::getGlobalInternal(const std::string& name) {
    if (!luaState_) return T();
    
    // lua_getglobal(luaState_, name.c_str());
    // T value = popFromStack<T>(luaState_);
    // lua_pop(luaState_, 1);
    // return value;
    
    return T();
}

template<typename T>
void ScriptComponent::setGlobalInternal(const std::string& name, T value) {
    if (!luaState_) return;
    
    // pushToStack(luaState_, value);
    // lua_setglobal(luaState_, name.c_str());
}

// Явные специализации для push/pop
void ScriptComponent::pushToStack(lua_State* L, int value) {
    // lua_pushinteger(L, value);
}

void ScriptComponent::pushToStack(lua_State* L, float value) {
    // lua_pushnumber(L, value);
}

void ScriptComponent::pushToStack(lua_State* L, double value) {
    // lua_pushnumber(L, value);
}

void ScriptComponent::pushToStack(lua_State* L, const std::string& value) {
    // lua_pushstring(L, value.c_str());
}

void ScriptComponent::pushToStack(lua_State* L, bool value) {
    // lua_pushboolean(L, value ? 1 : 0);
}

void ScriptComponent::pushToStack(lua_State* L, glm::vec3 value) {
    // Создание таблицы {x=value.x, y=value.y, z=value.z}
    // lua_newtable(L);
    // lua_pushnumber(L, value.x); lua_setfield(L, -2, "x");
    // lua_pushnumber(L, value.y); lua_setfield(L, -2, "y");
    // lua_pushnumber(L, value.z); lua_setfield(L, -2, "z");
}

void ScriptComponent::pushToStack(lua_State* L, Actor* actor) {
    // userdata для Actor
    // void* ud = lua_newuserdata(L, sizeof(Actor*));
    // *(Actor**)ud = actor;
    // luaL_getmetatable(L, "Actor");
    // lua_setmetatable(L, -2);
}

int ScriptComponent::popFromStack(lua_State* L) {
    // return lua_tointeger(L, -1);
    return 0;
}

float ScriptComponent::popFloatFromStack(lua_State* L) {
    // return lua_tonumber(L, -1);
    return 0.0f;
}

std::string ScriptComponent::popStringFromStack(lua_State* L) {
    // return lua_tostring(L, -1);
    return "";
}

bool ScriptComponent::popBoolFromStack(lua_State* L) {
    // return lua_toboolean(L, -1);
    return false;
}

glm::vec3 ScriptComponent::popVec3FromStack(lua_State* L) {
    // Извлечение таблицы {x, y, z}
    return glm::vec3(0);
}

// ============================================================================
// ScriptManager
// ============================================================================

ScriptManager::ScriptManager() = default;

ScriptManager::~ScriptManager() {
    Shutdown();
}

bool ScriptManager::Initialize(bool loadStandardLibs) {
    if (initialized_) {
        return true;
    }
    
    EOA_LOG_INFO("ScriptManager: Initializing Lua");
    
    // Создание глобального Lua state
    // globalState_ = luaL_newstate();
    
    if (!globalState_) {
        EOA_LOG_ERROR("ScriptManager: Failed to create Lua state");
        return false;
    }
    
    if (loadStandardLibs) {
        // luaL_openlibs(globalState_);
        EOA_LOG_INFO("ScriptManager: Loaded standard libraries");
    }
    
    // Регистрация стандартных классов
    // RegisterClass(std::make_unique<ScriptClass>("Actor"));
    // RegisterClass(std::make_unique<ScriptClass>("Component"));
    
    initialized_ = true;
    return true;
}

void ScriptManager::Shutdown() {
    if (!initialized_) return;
    
    EOA_LOG_INFO("ScriptManager: Shutting down Lua");
    
    // Выгрузка всех скриптов
    UnloadAllScripts();
    
    // Удаление зарегистрированных классов и модулей
    registeredClasses_.clear();
    registeredModules_.clear();
    
    // Закрытие Lua state
    if (globalState_) {
        // lua_close(globalState_);
        globalState_ = nullptr;
    }
    
    initialized_ = false;
}

void ScriptManager::RegisterClass(std::unique_ptr<ScriptClass> scriptClass) {
    if (!scriptClass) return;
    
    std::string name = scriptClass->GetName();
    registeredClasses_[name] = std::move(scriptClass);
    
    if (globalState_) {
        registeredClasses_[name]->Register(globalState_);
    }
}

ScriptClass* ScriptManager::FindClass(const std::string& name) {
    auto it = registeredClasses_.find(name);
    if (it != registeredClasses_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ScriptManager::RegisterModule(std::unique_ptr<ScriptModule> module) {
    if (!module) return;
    
    std::string name = module->GetName();
    registeredModules_[name] = std::move(module);
    
    if (globalState_) {
        registeredModules_[name]->Register(globalState_);
    }
}

bool ScriptManager::LoadScript(const std::string& filename, ScriptComponent* owner) {
    EOA_LOG_INFO("ScriptManager: Loading script '{}'", filename);
    
    // Чтение файла
    std::ifstream file(filename);
    if (!file.is_open()) {
        EOA_LOG_ERROR("ScriptManager: Failed to open script '{}'", filename);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    
    loadedScripts_[filename] = code;
    
    // Загрузка в Lua
    if (globalState_) {
        // luaL_loadbuffer(globalState_, code.c_str(), code.size(), filename.c_str());
        // lua_pcall(globalState_, 0, 0, 0);
    }
    
    return true;
}

bool ScriptManager::LoadScriptFromString(const std::string& code, const std::string& name) {
    EOA_LOG_INFO("ScriptManager: Loading inline script '{}'", name);
    
    loadedScripts_[name] = code;
    
    if (globalState_) {
        // luaL_loadbuffer(globalState_, code.c_str(), code.size(), name.c_str());
        // lua_pcall(globalState_, 0, 0, 0);
    }
    
    return true;
}

void ScriptManager::UnloadScript(const std::string& filename) {
    auto it = loadedScripts_.find(filename);
    if (it != loadedScripts_.end()) {
        loadedScripts_.erase(it);
        EOA_LOG_INFO("ScriptManager: Unloaded script '{}'", filename);
    }
}

void ScriptManager::UnloadAllScripts() {
    loadedScripts_.clear();
    EOA_LOG_INFO("ScriptManager: Unloaded all scripts");
}

void ScriptManager::ReloadAllScripts() {
    EOA_LOG_INFO("ScriptManager: Reloading all scripts");
    
    // Сохранение кода
    auto scriptsCopy = loadedScripts_;
    
    // Выгрузка
    loadedScripts_.clear();
    
    // Загрузка заново
    for (const auto& [filename, code] : scriptsCopy) {
        LoadScriptFromString(code, filename);
    }
}

bool ScriptManager::ExecuteCode(const std::string& code) {
    if (!globalState_) return false;
    
    EOA_LOG_INFO("ScriptManager: Executing code");
    
    // luaL_loadbuffer(globalState_, code.c_str(), code.size(), "=inline");
    // int result = lua_pcall(globalState_, 0, LUA_MULTRET, 0);
    
    // return result == 0;
    return true;
}

void ScriptManager::AddSearchPath(const std::string& path) {
    searchPaths_.push_back(path);
    
    if (globalState_) {
        // Обновление package.path в Lua
        // std::string newPath = path + "/?.lua;" + lua_tostring(globalState_, -1);
        // lua_pushstring(globalState_, newPath.c_str());
        // lua_setfield(globalState_, -2, "path");
    }
}

void ScriptManager::LoadStandardLibrary(const std::string& name) {
    EOA_LOG_INFO("ScriptManager: Loading standard library '{}'", name);
    
    // Загрузка стандартной библиотеки Lua (math, string, table, etc.)
    // В реальной реализации через require
}

void ScriptManager::GarbageCollect() {
    if (!globalState_) return;
    
    // lua_gc(globalState_, LUA_GCCOLLECT, 0);
}

void ScriptManager::SetGCThreshold(int threshold) {
    gcThreshold_ = threshold;
    
    if (globalState_) {
        // lua_gc(globalState_, LUA_GCSETPAUSE, threshold);
    }
}

// ============================================================================
// ScriptUtils
// ============================================================================

namespace ScriptUtils {

bool IsNil(lua_State* L, int index) {
    return lua_isnil(L, index) != 0;
}

bool IsNumber(lua_State* L, int index) {
    return lua_isnumber(L, index) != 0;
}

bool IsString(lua_State* L, int index) {
    return lua_isstring(L, index) != 0;
}

bool IsTable(lua_State* L, int index) {
    return lua_istable(L, index) != 0;
}

bool IsFunction(lua_State* L, int index) {
    return lua_isfunction(L, index) != 0;
}

bool IsUserdata(lua_State* L, int index) {
    return lua_isuserdata(L, index) != 0;
}

int SafeGetInteger(lua_State* L, int index, int defaultValue) {
    if (lua_isnumber(L, index)) {
        return lua_tointeger(L, index);
    }
    return defaultValue;
}

float SafeGetNumber(lua_State* L, int index, float defaultValue) {
    if (lua_isnumber(L, index)) {
        return static_cast<float>(lua_tonumber(L, index));
    }
    return defaultValue;
}

std::string SafeGetString(lua_State* L, int index, const std::string& defaultValue) {
    if (lua_isstring(L, index)) {
        return lua_tostring(L, index);
    }
    return defaultValue;
}

bool SafeGetBool(lua_State* L, int index, bool defaultValue) {
    if (lua_isboolean(L, index)) {
        return lua_toboolean(L, index) != 0;
    }
    return defaultValue;
}

std::string GetLastError(lua_State* L) {
    if (lua_isstring(L, -1)) {
        return lua_tostring(L, -1);
    }
    return "Unknown error";
}

void PrintStackTrace(lua_State* L) {
    // lua_Debug ar;
    // int level = 1;
    // while (lua_getstack(L, level++, &ar)) {
    //     lua_getinfo(L, "Sln", &ar);
    //     EOA_LOG_INFO("  {}:{} in {}", ar.source, ar.currentline, ar.name);
    // }
}

int LoadFile(lua_State* L, const std::string& filename) {
    return luaL_loadfile(L, filename.c_str());
}

} // namespace ScriptUtils

} // namespace eoa
