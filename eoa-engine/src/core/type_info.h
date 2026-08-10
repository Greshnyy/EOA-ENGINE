#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <any>
#include <optional>
#include <variant>
#include <type_traits>
#include <limits>
#include "core/object.h"

namespace eoa {

// Forward declarations
class Property;
class Function;
class Enum;
class Class;
class ReflectionSystem;

enum class PropertyType {
    None, Bool, Int, Int8, Int16, Int64, UInt, UInt8, UInt16, UInt64,
    Float, Double, String, Vec2, Vec3, Vec4, Quat, Mat2, Mat3, Mat4,
    Color, Object, Enum, Array, Map, Custom, Function
};

inline const char* PropertyTypeToString(PropertyType type) {
    switch (type) {
        case PropertyType::Bool: return "Bool";
        case PropertyType::Int: return "Int";
        case PropertyType::Int8: return "Int8";
        case PropertyType::Int16: return "Int16";
        case PropertyType::Int64: return "Int64";
        case PropertyType::UInt: return "UInt";
        case PropertyType::UInt8: return "UInt8";
        case PropertyType::UInt16: return "UInt16";
        case PropertyType::UInt64: return "UInt64";
        case PropertyType::Float: return "Float";
        case PropertyType::Double: return "Double";
        case PropertyType::String: return "String";
        case PropertyType::Vec2: return "Vec2";
        case PropertyType::Vec3: return "Vec3";
        case PropertyType::Vec4: return "Vec4";
        case PropertyType::Quat: return "Quat";
        case PropertyType::Mat2: return "Mat2";
        case PropertyType::Mat3: return "Mat3";
        case PropertyType::Mat4: return "Mat4";
        case PropertyType::Color: return "Color";
        case PropertyType::Object: return "Object";
        case PropertyType::Enum: return "Enum";
        case PropertyType::Array: return "Array";
        case PropertyType::Map: return "Map";
        case PropertyType::Custom: return "Custom";
        case PropertyType::Function: return "Function";
        default: return "None";
    }
}

struct PropertyMetadata {
    std::string displayName, category, tooltip;
    float minValue = std::numeric_limits<float>::lowest();
    float maxValue = std::numeric_limits<float>::max();
    float step = 0.01f;
    bool readOnly = false, visible = true, advanced = false;
    std::string enumType, arrayElementType;
    std::function<bool(const std::any&)> validator;
    std::string group;
    int sortOrder = 0;
    bool isColor = false, hasAlpha = true;
    bool isFilePath = false;
    std::string fileFilter;
    bool multiline = false;
    int multilineRows = 3;
};

struct FunctionMetadata {
    std::string displayName, category, tooltip;
    bool isPure = false, isStatic = false;
    std::vector<std::string> paramNames, paramTooltips;
    std::string returnType;
};

class Property {
public:
    using GetterFunc = std::function<std::any(void*)>;
    using SetterFunc = std::function<void(void*, const std::any&)>;
    Property(const std::string& name, PropertyType type, GetterFunc getter, SetterFunc setter, const PropertyMetadata& metadata = {})
        : name_(name), type_(type), getter_(std::move(getter)), setter_(std::move(setter)), metadata_(metadata) {}
    const std::string& GetName() const { return name_; }
    PropertyType GetType() const { return type_; }
    const PropertyMetadata& GetMetadata() const { return metadata_; }
    const std::string& GetTypeName() const { return typeName_; }
    void SetTypeName(const std::string& name) { typeName_ = name; }
    std::any Get(void* instance) const { return getter_ && instance ? getter_(instance) : std::any(); }
    bool Set(void* instance, const std::any& value) const {
        if (!setter_ || !instance || metadata_.readOnly) return false;
        try { setter_(instance, value); return true; } catch (const std::bad_any_cast&) { return false; }
    }
    template<typename T> bool IsType() const {
        if constexpr (std::is_same_v<T,bool>) return type_==PropertyType::Bool;
        else if constexpr (std::is_same_v<T,int>) return type_==PropertyType::Int;
        else if constexpr (std::is_same_v<T,int8_t>) return type_==PropertyType::Int8;
        else if constexpr (std::is_same_v<T,int16_t>) return type_==PropertyType::Int16;
        else if constexpr (std::is_same_v<T,int64_t>) return type_==PropertyType::Int64;
        else if constexpr (std::is_same_v<T,uint32_t>) return type_==PropertyType::UInt;
        else if constexpr (std::is_same_v<T,uint8_t>) return type_==PropertyType::UInt8;
        else if constexpr (std::is_same_v<T,uint16_t>) return type_==PropertyType::UInt16;
        else if constexpr (std::is_same_v<T,uint64_t>) return type_==PropertyType::UInt64;
        else if constexpr (std::is_same_v<T,float>) return type_==PropertyType::Float;
        else if constexpr (std::is_same_v<T,double>) return type_==PropertyType::Double;
        else if constexpr (std::is_same_v<T,std::string>) return type_==PropertyType::String;
        else if constexpr (std::is_same_v<T,glm::vec2>) return type_==PropertyType::Vec2;
        else if constexpr (std::is_same_v<T,glm::vec3>) return type_==PropertyType::Vec3;
        else if constexpr (std::is_same_v<T,glm::vec4>) return type_==PropertyType::Vec4;
        else if constexpr (std::is_same_v<T,glm::quat>) return type_==PropertyType::Quat;
        else if constexpr (std::is_same_v<T,glm::mat2>) return type_==PropertyType::Mat2;
        else if constexpr (std::is_same_v<T,glm::mat3>) return type_==PropertyType::Mat3;
        else if constexpr (std::is_same_v<T,glm::mat4>) return type_==PropertyType::Mat4;
        else return false;
    }
    bool IsArray() const { return type_ == PropertyType::Array; }
    bool IsEnum() const { return type_ == PropertyType::Enum; }
    const std::string& GetEnumType() const { return metadata_.enumType; }
private:
    std::string name_, typeName_;
    PropertyType type_;
    GetterFunc getter_;
    SetterFunc setter_;
    PropertyMetadata metadata_;
};

template<typename T> std::unique_ptr<Property> MakeProperty(const std::string& name, PropertyType type, T* member) {
    return std::make_unique<Property>(name, type,
        [member](void* instance) { return std::any(static_cast<T*>(instance)->*member); },
        [member](void* instance, const std::any& value) { static_cast<T*>(instance)->*member = std::any_cast<decltype(static_cast<T*>(instance)->*member)>(value); });
}

class Function {
public:
    using FuncPtr = std::function<std::any(void*, const std::vector<std::any>&)>;
    Function(const std::string& name, FuncPtr func, const FunctionMetadata& metadata = {}) : name_(name), func_(std::move(func)), metadata_(metadata) {}
    const std::string& GetName() const { return name_; }
    const FunctionMetadata& GetMetadata() const { return metadata_; }
    std::any Invoke(void* instance, const std::vector<std::any>& params = {}) const { return func_ ? func_(instance, params) : std::any(); }
    size_t GetParamCount() const { return metadata_.paramNames.size(); }
    const std::string& GetParamName(size_t index) const { static const std::string empty; return index < metadata_.paramNames.size() ? metadata_.paramNames[index] : empty; }
private:
    std::string name_;
    FuncPtr func_;
    FunctionMetadata metadata_;
};

class Enum {
public:
    explicit Enum(const std::string& name) : name_(name) {}
    const std::string& GetName() const { return name_; }
    void AddValue(const std::string& name, int64_t value) { values_[name]=value; valueToName_[value]=name; }
    std::optional<int64_t> GetValueByName(const std::string& name) const { auto it=values_.find(name); return it!=values_.end()?std::optional<int64_t>(it->second):std::nullopt; }
    std::string GetNameByValue(int64_t value) const { auto it=valueToName_.find(value); return it!=valueToName_.end()?it->second:""; }
    const std::unordered_map<std::string,int64_t>& GetValues() const { return values_; }
    size_t GetValueCount() const { return values_.size(); }
private:
    std::string name_;
    std::unordered_map<std::string,int64_t> values_;
    std::unordered_map<int64_t,std::string> valueToName_;
};

class Class {
public:
    using ConstructorFunc = std::function<std::unique_ptr<Object>()>;
    using DestructorFunc = std::function<void(Object*)>;
    Class(const std::string& name,const std::string& parentName="") : name_(name),parentName_(parentName) {}
    const std::string& GetName() const { return name_; }
    const std::string& GetParentName() const { return parentName_; }
    void SetSize(size_t size) { size_=size; }
    size_t GetSize() const { return size_; }
    void AddProperty(std::unique_ptr<Property> prop) { if(prop) properties_[prop->GetName()]=std::move(prop); }
    Property* GetProperty(const std::string& name) { auto it=properties_.find(name); return it!=properties_.end()?it->second.get():nullptr; }
    const Property* GetProperty(const std::string& name) const { auto it=properties_.find(name); return it!=properties_.end()?it->second.get():nullptr; }
    const std::unordered_map<std::string,std::unique_ptr<Property>>& GetProperties() const { return properties_; }
    void AddFunction(std::unique_ptr<Function> func) { if(func) functions_[func->GetName()]=std::move(func); }
    Function* GetFunction(const std::string& name) { auto it=functions_.find(name); return it!=functions_.end()?it->second.get():nullptr; }
    const std::unordered_map<std::string,std::unique_ptr<Function>>& GetFunctions() const { return functions_; }
    void SetConstructor(ConstructorFunc func) { constructor_=std::move(func); }
    void SetDestructor(DestructorFunc func) { destructor_=std::move(func); }
    std::unique_ptr<Object> CreateInstance() const { return constructor_ ? constructor_() : nullptr; }
    void DestroyInstance(Object* obj) const { if(destructor_&&obj) destructor_(obj); }
    bool IsChildOf(const std::string& className) const;
    template<typename T> bool IsChildOf() const { return IsChildOf(T::StaticClassName()); }
private:
    std::string name_, parentName_;
    size_t size_=0;
    std::unordered_map<std::string,std::unique_ptr<Property>> properties_;
    std::unordered_map<std::string,std::unique_ptr<Function>> functions_;
    ConstructorFunc constructor_;
    DestructorFunc destructor_;
};

class ReflectionSystem {
public:
    static ReflectionSystem& Get() { static ReflectionSystem instance; return instance; }
    void RegisterClass(std::unique_ptr<Class> cls) { if(cls) classes_[cls->GetName()]=std::move(cls); }
    Class* GetClass(const std::string& name) { auto it=classes_.find(name); return it!=classes_.end()?it->second.get():nullptr; }
    const Class* GetClass(const std::string& name) const { auto it=classes_.find(name); return it!=classes_.end()?it->second.get():nullptr; }
    std::unique_ptr<Object> CreateObject(const std::string& className) const { auto* cls=GetClass(className); return cls?cls->CreateInstance():nullptr; }
    void RegisterEnum(std::unique_ptr<Enum> e) { if(e) enums_[e->GetName()]=std::move(e); }
    Enum* GetEnum(const std::string& name) { auto it=enums_.find(name); return it!=enums_.end()?it->second.get():nullptr; }
private:
    std::unordered_map<std::string,std::unique_ptr<Class>> classes_;
    std::unordered_map<std::string,std::unique_ptr<Enum>> enums_;
};

} // namespace eoa
