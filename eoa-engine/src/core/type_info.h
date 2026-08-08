#pragma once
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

// ============================================================================
// Типы свойств для рефлексии
// ============================================================================
enum class PropertyType {
    None,
    Bool,
    Int,
    Int8,
    Int16,
    Int64,
    UInt,
    UInt8,
    UInt16,
    UInt64,
    Float,
    Double,
    String,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Mat2,
    Mat3,
    Mat4,
    Color,
    Object,
    Enum,
    Array,
    Map,
    Custom,
    Function
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

// ============================================================================
// Расширенные метаданные свойства
// ============================================================================
struct PropertyMetadata {
    std::string displayName;
    std::string category;
    std::string tooltip;
    float minValue = std::numeric_limits<float>::lowest();
    float maxValue = std::numeric_limits<float>::max();
    float step = 0.01f;
    bool readOnly = false;
    bool visible = true;
    bool advanced = false;  // Показывать только в расширенном режиме
    std::string enumType;   // Имя enum для PropertyType::Enum
    std::string arrayElementType;  // Тип элемента для массивов
    std::function<bool(const std::any&)> validator;  // Валидатор значения
    
    // Группировка в редакторе
    std::string group;
    int sortOrder = 0;
    
    // Для цветов
    bool isColor = false;
    bool hasAlpha = true;
    
    // Для путей к файлам
    bool isFilePath = false;
    std::string fileFilter;  // Например: "*.png;*.jpg"
    
    // Для multiline строк
    bool multiline = false;
    int multilineRows = 3;
};

// ============================================================================
// Метаданные функции
// ============================================================================
struct FunctionMetadata {
    std::string displayName;
    std::string category;
    std::string tooltip;
    bool isPure = false;      // Const функция
    bool isStatic = false;    // Статический метод
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTooltips;
    std::string returnType;
};

// ============================================================================
// Класс Property представляет отдельное свойство компонента
// ============================================================================
class Property {
public:
    using GetterFunc = std::function<std::any(void*)>;
    using SetterFunc = std::function<void(void*, const std::any&)>;

    Property(const std::string& name, PropertyType type, GetterFunc getter, SetterFunc setter, 
             const PropertyMetadata& metadata = PropertyMetadata())
        : name_(name)
        , type_(type)
        , getter_(getter)
        , setter_(setter)
        , metadata_(metadata)
    {}

    const std::string& GetName() const { return name_; }
    PropertyType GetType() const { return type_; }
    const PropertyMetadata& GetMetadata() const { return metadata_; }
    
    // Получить строковое имя типа (для enum, array и т.д.)
    const std::string& GetTypeName() const { return typeName_; }
    void SetTypeName(const std::string& name) { typeName_ = name; }

    // Получить значение свойства из экземпляра объекта
    std::any Get(void* instance) const {
        if (getter_ && instance) {
            return getter_(instance);
        }
        return std::any();
    }

    // Установить значение свойства в экземпляре объекта
    bool Set(void* instance, const std::any& value) const {
        if (setter_ && instance && !metadata_.readOnly) {
            try {
                setter_(instance, value);
                return true;
            } catch (const std::bad_any_cast&) {
                return false;
            }
        }
        return false;
    }

    // Проверка типа
    template<typename T>
    bool IsType() const {
        if constexpr (std::is_same_v<T, bool>) return type_ == PropertyType::Bool;
        else if constexpr (std::is_same_v<T, int>) return type_ == PropertyType::Int;
        else if constexpr (std::is_same_v<T, int8_t>) return type_ == PropertyType::Int8;
        else if constexpr (std::is_same_v<T, int16_t>) return type_ == PropertyType::Int16;
        else if constexpr (std::is_same_v<T, int64_t>) return type_ == PropertyType::Int64;
        else if constexpr (std::is_same_v<T, uint32_t>) return type_ == PropertyType::UInt;
        else if constexpr (std::is_same_v<T, uint8_t>) return type_ == PropertyType::UInt8;
        else if constexpr (std::is_same_v<T, uint16_t>) return type_ == PropertyType::UInt16;
        else if constexpr (std::is_same_v<T, uint64_t>) return type_ == PropertyType::UInt64;
        else if constexpr (std::is_same_v<T, float>) return type_ == PropertyType::Float;
        else if constexpr (std::is_same_v<T, double>) return type_ == PropertyType::Double;
        else if constexpr (std::is_same_v<T, std::string>) return type_ == PropertyType::String;
        return false;
    }
    
    // Проверка на массив
    bool IsArray() const { return type_ == PropertyType::Array; }
    
    // Проверка на enum
    bool IsEnum() const { return type_ == PropertyType::Enum; }
    const std::string& GetEnumType() const { return metadata_.enumType; }

private:
    std::string name_;
    PropertyType type_;
    std::string typeName_;  // Для custom типов, enum, array element type
    GetterFunc getter_;
    SetterFunc setter_;
    PropertyMetadata metadata_;
};

// ============================================================================
// Класс Function для отражения методов
// ============================================================================
class Function {
public:
    using FuncPtr = std::function<std::any(void*, const std::vector<std::any>&)>;

    Function(const std::string& name, FuncPtr func, const FunctionMetadata& metadata = FunctionMetadata())
        : name_(name)
        , func_(func)
        , metadata_(metadata)
    {}

    const std::string& GetName() const { return name_; }
    const FunctionMetadata& GetMetadata() const { return metadata_; }
    
    // Вызов функции
    std::any Invoke(void* instance, const std::vector<std::any>& params = {}) const {
        if (func_) {
            return func_(instance, params);
        }
        return std::any();
    }
    
    // Количество параметров
    size_t GetParamCount() const { return metadata_.paramNames.size(); }
    
    // Имя параметра по индексу
    const std::string& GetParamName(size_t index) const {
        static const std::string empty;
        return index < metadata_.paramNames.size() ? metadata_.paramNames[index] : empty;
    }

private:
    std::string name_;
    FuncPtr func_;
    FunctionMetadata metadata_;
};

// ============================================================================
// Класс Enum для отражения перечислений
// ============================================================================
class Enum {
public:
    Enum(const std::string& name)
        : name_(name)
    {}

    const std::string& GetName() const { return name_; }
    
    // Добавить значение enum
    void AddValue(const std::string& name, int64_t value) {
        values_[name] = value;
        valueToName_[value] = name;
    }
    
    // Получить значение по имени
    std::optional<int64_t> GetValueByName(const std::string& name) const {
        auto it = values_.find(name);
        if (it != values_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Получить имя по значению
    std::string GetNameByValue(int64_t value) const {
        auto it = valueToName_.find(value);
        if (it != valueToName_.end()) {
            return it->second;
        }
        return "";
    }
    
    // Получить все имена
    const std::unordered_map<std::string, int64_t>& GetValues() const {
        return values_;
    }
    
    // Получить количество значений
    size_t GetValueCount() const { return values_.size(); }

private:
    std::string name_;
    std::unordered_map<std::string, int64_t> values_;
    std::unordered_map<int64_t, std::string> valueToName_;
};

// ============================================================================
// Класс Class представляет тип класса с его свойствами и методами
// ============================================================================
class Class {
public:
    using ConstructorFunc = std::function<std::unique_ptr<Object>()>;
    using DestructorFunc = std::function<void(Object*)>;

    Class(const std::string& name, const std::string& parentName = "")
        : name_(name)
        , parentName_(parentName)
        , size_(0)
    {}

    const std::string& GetName() const { return name_; }
    const std::string& GetParentName() const { return parentName_; }
    
    // Установить размер класса в байтах
    void SetSize(size_t size) { size_ = size; }
    size_t GetSize() const { return size_; }

    // Добавить свойство
    void AddProperty(std::unique_ptr<Property> prop) {
        properties_[prop->GetName()] = std::move(prop);
    }

    // Получить свойство по имени
    Property* GetProperty(const std::string& name) {
        auto it = properties_.find(name);
        return it != properties_.end() ? it->second.get() : nullptr;
    }

    const Property* GetProperty(const std::string& name) const {
        auto it = properties_.find(name);
        return it != properties_.end() ? it->second.get() : nullptr;
    }

    // Получить все свойства
    const std::unordered_map<std::string, std::unique_ptr<Property>>& GetProperties() const {
        return properties_;
    }
    
    // Добавить функцию
    void AddFunction(std::unique_ptr<Function> func) {
        functions_[func->GetName()] = std::move(func);
    }
    
    // Получить функцию по имени
    Function* GetFunction(const std::string& name) {
        auto it = functions_.find(name);
        return it != functions_.end() ? it->second.get() : nullptr;
    }
    
    const Function* GetFunction(const std::string& name) const {
        auto it = functions_.find(name);
        return it != functions_.end() ? it->second.get() : nullptr;
    }
    
    // Получить все функции
    const std::unordered_map<std::string, std::unique_ptr<Function>>& GetFunctions() const {
        return functions_;
    }

    // Установить функцию конструктора
    void SetConstructor(ConstructorFunc func) {
        constructor_ = func;
    }
    
    // Установить функцию деструктора
    void SetDestructor(DestructorFunc func) {
        destructor_ = func;
    }

    // Создать экземпляр класса
    std::unique_ptr<Object> CreateInstance() const {
        if (constructor_) {
            return constructor_();
        }
        return nullptr;
    }
    
    // Уничтожить экземпляр
    void DestroyInstance(Object* obj) const {
        if (destructor_ && obj) {
            destructor_(obj);
        }
    }

    // Проверка наследования
    bool IsChildOf(const std::string& className) const {
        if (name_ == className) return true;
        if (parentName_.empty()) return false;
        
        auto parentClass = ReflectionSystem::Get().GetClass(parentName_);
        if (!parentClass) return false;
        
        return parentClass->IsChildOf(className);
    }

    template<typename T>
    bool IsChildOf() const {
        return IsChildOf(T::StaticClassName());
    }
    
    // Получить все родительские классы
    std::vector<std::string> GetAllParentClasses() const {
        std::vector<std::string> parents;
        std::string current = parentName_;
        
        while (!current.empty()) {
            parents.push_back(current);
            auto parentClass = ReflectionSystem::Get().GetClass(current);
            if (!parentClass) break;
            current = parentClass->GetParentName();
        }
        
        return parents;
    }

private:
    std::string name_;
    std::string parentName_;
    size_t size_;
    std::unordered_map<std::string, std::unique_ptr<Property>> properties_;
    std::unordered_map<std::string, std::unique_ptr<Function>> functions_;
    ConstructorFunc constructor_;
    DestructorFunc destructor_;
};

// ============================================================================
// Глобальная система рефлексии
// ============================================================================
class ReflectionSystem {
public:
    static ReflectionSystem& Get() {
        static ReflectionSystem instance;
        return instance;
    }

    // Регистрация класса
    void RegisterClass(std::unique_ptr<Class> cls) {
        classes_[cls->GetName()] = std::move(cls);
    }

    // Получить класс по имени
    Class* GetClass(const std::string& name) {
        auto it = classes_.find(name);
        return it != classes_.end() ? it->second.get() : nullptr;
    }

    const Class* GetClass(const std::string& name) const {
        auto it = classes_.find(name);
        return it != classes_.end() ? it->second.get() : nullptr;
    }

    // Получить все зарегистрированные классы
    const std::unordered_map<std::string, std::unique_ptr<Class>>& GetClasses() const {
        return classes_;
    }

    // Создать экземпляр по имени класса
    std::unique_ptr<Object> CreateObject(const std::string& className) {
        auto cls = GetClass(className);
        if (cls) {
            return cls->CreateInstance();
        }
        return nullptr;
    }
    
    // Регистрация enum
    void RegisterEnum(std::unique_ptr<Enum> enm) {
        enums_[enm->GetName()] = std::move(enm);
    }
    
    // Получить enum по имени
    Enum* GetEnum(const std::string& name) {
        auto it = enums_.find(name);
        return it != enums_.end() ? it->second.get() : nullptr;
    }
    
    const Enum* GetEnum(const std::string& name) const {
        auto it = enums_.find(name);
        return it != enums_.end() ? it->second.get() : nullptr;
    }
    
    // Получить все enum
    const std::unordered_map<std::string, std::unique_ptr<Enum>>& GetEnums() const {
        return enums_;
    }
    
    // Найти класс по базовому классу
    std::vector<Class*> GetClassesByBase(const std::string& baseClassName) const {
        std::vector<Class*> result;
        for (auto& [name, cls] : classes_) {
            if (cls->IsChildOf(baseClassName)) {
                result.push_back(cls.get());
            }
        }
        return result;
    }
    
    // Шаблонный поиск классов по базовому типу
    template<typename BaseT>
    std::vector<Class*> GetClassesByBase() const {
        return GetClassesByBase(BaseT::StaticClassName());
    }

private:
    ReflectionSystem() = default;
    std::unordered_map<std::string, std::unique_ptr<Class>> classes_;
    std::unordered_map<std::string, std::unique_ptr<Enum>> enums_;
};

// ============================================================================
// Helper функции для создания Property с лямбда-функциями
// ============================================================================

// Для member pointer
template<typename OwnerType, typename ValueType>
std::unique_ptr<Property> MakeProperty(
    const std::string& name,
    PropertyType type,
    ValueType OwnerType::*member,
    const PropertyMetadata& metadata = PropertyMetadata())
{
    auto getter = [member](void* obj) -> std::any {
        auto instance = static_cast<OwnerType*>(obj);
        return std::any(instance->*member);
    };

    auto setter = [member](void* obj, const std::any& value) {
        auto instance = static_cast<OwnerType*>(obj);
        try {
            instance->*member = std::any_cast<ValueType>(value);
        } catch (const std::bad_any_cast&) {
            // Игнорируем несовместимые типы
        }
    };

    return std::make_unique<Property>(name, type, getter, setter, metadata);
}

// Для getter/setter функций
template<typename OwnerType, typename ValueType>
std::unique_ptr<Property> MakeProperty(
    const std::string& name,
    PropertyType type,
    ValueType (OwnerType::*getter)() const,
    void (OwnerType::*setter)(ValueType),
    const PropertyMetadata& metadata = PropertyMetadata())
{
    auto getFunc = [getter](void* obj) -> std::any {
        auto instance = static_cast<OwnerType*>(obj);
        if (getter) {
            return std::any((instance->*getter)());
        }
        return std::any();
    };

    auto setFunc = [setter](void* obj, const std::any& value) {
        auto instance = static_cast<OwnerType*>(obj);
        if (setter) {
            try {
                (instance->*setter)(std::any_cast<ValueType>(value));
            } catch (const std::bad_any_cast&) {
                // Игнорируем несовместимые типы
            }
        }
    };

    return std::make_unique<Property>(name, type, getFunc, setFunc, metadata);
}

// Только getter (read-only property)
template<typename OwnerType, typename ValueType>
std::unique_ptr<Property> MakeReadOnlyProperty(
    const std::string& name,
    PropertyType type,
    ValueType (OwnerType::*getter)() const,
    const PropertyMetadata& metadata = PropertyMetadata())
{
    auto getFunc = [getter](void* obj) -> std::any {
        auto instance = static_cast<OwnerType*>(obj);
        if (getter) {
            return std::any((instance->*getter)());
        }
        return std::any();
    };

    auto prop = std::make_unique<Property>(name, type, getFunc, nullptr, metadata);
    return prop;
}

// ============================================================================
// Helper для создания Function
// ============================================================================
template<typename OwnerType, typename ReturnType, typename... Params>
std::unique_ptr<Function> MakeFunction(
    const std::string& name,
    ReturnType (OwnerType::*method)(Params...),
    const FunctionMetadata& metadata = FunctionMetadata())
{
    auto func = [method](void* obj, const std::vector<std::any>& params) -> std::any {
        auto instance = static_cast<OwnerType*>(obj);
        if constexpr (std::is_void_v<ReturnType>) {
            if (instance && method) {
                (instance->*method)();
            }
            return std::any();
        } else {
            if (instance && method) {
                return std::any((instance->*method)());
            }
            return std::any();
        }
    };
    
    return std::make_unique<Function>(name, func, metadata);
}

// ============================================================================
// Helper для регистрации Enum
// ============================================================================
template<typename EnumType>
std::unique_ptr<Enum> MakeEnum(const std::string& name) {
    auto enm = std::make_unique<Enum>(name);
    return enm;
}

} // namespace eoa
