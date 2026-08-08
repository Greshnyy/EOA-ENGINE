#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <any>
#include "core/eoa_types.h" // Убедитесь, что пути верные
#include <glm/glm.hpp>

namespace eoa {

// Forward declarations
class Object;
class Property;

// Типы свойств
enum class PropertyType {
    None,
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Mat3,
    Mat4,
    ObjectRef,
    Enum
};

// Метаданные свойства
struct PropertyMetadata {
    float minVal = -std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::max();
    float step = 0.1f;
    bool isReadOnly = false;
    std::string category;
    std::string tooltip;
};

// Базовый класс свойства
class Property {
public:
    Property(const std::string& name, PropertyType type, const PropertyMetadata& meta = {})
        : name_(name), type_(type), metadata_(meta) {}
    virtual ~Property() = default;

    const std::string& GetName() const { return name_; }
    PropertyType GetType() const { return type_; }
    const PropertyMetadata& GetMetadata() const { return metadata_; }

    virtual std::any GetValue(Object* obj) const = 0;
    virtual void SetValue(Object* obj, const std::any& value) = 0;
    virtual std::string GetValueAsString(Object* obj) const = 0;

private:
    std::string name_;
    PropertyType type_;
    PropertyMetadata metadata_;
};

// Шаблонный класс свойства
template<typename T>
class TypedProperty : public Property {
public:
    using Getter = T (*)(Object*);
    using Setter = void (*)(Object*, T);
    using MemberPtr = T Object::*;

    TypedProperty(const std::string& name, PropertyType type, Getter getter, Setter setter, const PropertyMetadata& meta = {})
        : Property(name, type, meta), getter_(getter), setter_(setter), memberPtr_(nullptr) {}

    TypedProperty(const std::string& name, PropertyType type, MemberPtr ptr, const PropertyMetadata& meta = {})
        : Property(name, type, meta), getter_(nullptr), setter_(nullptr), memberPtr_(ptr) {}

    std::any GetValue(Object* obj) const override {
        if (memberPtr_) {
            return obj->*memberPtr_;
        } else if (getter_) {
            return getter_(obj);
        }
        return std::any();
    }

    void SetValue(Object* obj, const std::any& value) override {
        if (setter_) {
            try {
                setter_(obj, std::any_cast<T>(value));
            } catch (...) {}
        } else if (memberPtr_) {
            try {
                obj->*memberPtr_ = std::any_cast<T>(value);
            } catch (...) {}
        }
    }

    std::string GetValueAsString(Object* obj) const override {
        auto val = GetValue(obj);
        if (val.has_value()) {
            // Простая реализация для примера
            return "Value"; 
        }
        return "None";
    }

private:
    Getter getter_;
    Setter setter_;
    MemberPtr memberPtr_;
};

// Класс типа (Class Info)
class ClassInfo {
public:
    using CreateFunc = Object*(*)();

    ClassInfo(const std::string& name, CreateFunc createFunc = nullptr)
        : name_(name), createFunc_(createFunc) {}

    void AddProperty(std::unique_ptr<Property> prop) {
        properties_[prop->GetName()] = std::move(prop);
    }

    Property* GetProperty(const std::string& name) {
        auto it = properties_.find(name);
        return it != properties_.end() ? it->second.get() : nullptr;
    }

    const std::string& GetName() const { return name_; }
    Object* CreateInstance() const { return createFunc_ ? createFunc_() : nullptr; }

private:
    std::string name_;
    CreateFunc createFunc_;
    std::unordered_map<std::string, std::unique_ptr<Property>> properties_;
};

// Менеджер отражений
class ReflectionManager {
public:
    static ReflectionManager& Get() {
        static ReflectionManager instance;
        return instance;
    }

    void RegisterClass(std::unique_ptr<ClassInfo> info) {
        classes_[info->GetName()] = std::move(info);
    }

    ClassInfo* GetClass(const std::string& name) {
        auto it = classes_.find(name);
        return it != classes_.end() ? it->second.get() : nullptr;
    }

private:
    ReflectionManager() = default;
    std::unordered_map<std::string, std::unique_ptr<ClassInfo>> classes_;
};

// Helper функции для создания свойств
template<typename OwnerType, typename ValueType>
std::unique_ptr<Property> MakeProperty(
    const std::string& name,
    PropertyType type,
    ValueType OwnerType::* ptr,
    const PropertyMetadata& meta = {}
) {
    return std::make_unique<TypedProperty<ValueType>>(name, type, ptr, meta);
}

template<typename OwnerType, typename ValueType>
std::unique_ptr<Property> MakeProperty(
    const std::string& name,
    PropertyType type,
    ValueType (*getter)(OwnerType*),
    void (*setter)(OwnerType*, ValueType),
    const PropertyMetadata& meta = {}
) {
    // Требует адаптации под сигнатуры, упрощено для примера
    // В реальной реализации нужно привести к базовому типу Object*
    return nullptr; 
}

} // namespace eoa

// Макросы для упрощения регистрации
#define EOA_CLASS(Classname) \
    static eoa::ClassInfo* Register##Classname() { \
        auto info = std::make_unique<eoa::ClassInfo>(#Classname, []() -> eoa::Object* { return new Classname(); });

#define EOA_PROPERTY(Name, Type, Ptr) \
    info->AddProperty(eoa::MakeProperty<Classname>(Name, Type, Ptr));

#define EOA_END_CLASS() \
        eoa::ReflectionManager::Get().RegisterClass(std::move(info)); \
        return nullptr; \
    }

// Специализации IsType (исправление ошибки компиляции)
namespace eoa {
    template<typename T>
    bool IsType(PropertyType pt) { return false; }
    
    // Явные специализации будут в cpp или здесь
    template<> inline bool IsType<bool>(PropertyType pt) { return pt == PropertyType::Bool; }
    template<> inline bool IsType<int32>(PropertyType pt) { return pt == PropertyType::Int; }
    template<> inline bool IsType<float>(PropertyType pt) { return pt == PropertyType::Float; }
    template<> inline bool IsType<glm::vec2>(PropertyType pt) { return pt == PropertyType::Vec2; }
    template<> inline bool IsType<glm::vec3>(PropertyType pt) { return pt == PropertyType::Vec3; }
    template<> inline bool IsType<glm::vec4>(PropertyType pt) { return pt == PropertyType::Vec4; }
    template<> inline bool IsType<glm::quat>(PropertyType pt) { return pt == PropertyType::Quat; }
    template<> inline bool IsType<glm::mat3>(PropertyType pt) { return pt == PropertyType::Mat3; }
    template<> inline bool IsType<glm::mat4>(PropertyType pt) { return pt == PropertyType::Mat4; }
}
