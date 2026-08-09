#pragma once
#include "core/type_info.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/color_space.hpp>

// ============================================================================
// Макросы для объявления рефлектируемых классов
// ============================================================================
#define EOA_CLASS_DECL(Classname, ParentClass) \
public: \
    static const char* StaticClassName() { return #Classname; } \
    const char* ClassName() const override { return #Classname; } \
    static void RegisterReflection(); \
    static eoa::Class* StaticClass() { \
        static eoa::Class* cls = nullptr; \
        if (!cls) { \
            cls = eoa::ReflectionSystem::Get().GetClass(#Classname); \
        } \
        return cls; \
    }

// ============================================================================
// Макросы для определения рефлектируемых классов
// ============================================================================
#define EOA_CLASS_IMPL(Classname, ParentClass) \
    void Classname::RegisterReflection() { \
        auto cls = std::make_unique<eoa::Class>(#Classname, #ParentClass); \
        cls->SetSize(sizeof(Classname)); \
        cls->SetConstructor([]() -> std::unique_ptr<eoa::Object> { \
            return std::make_unique<Classname>(); \
        });

// Базовый макрос для свойства
#define EOA_PROPERTY(Member, Type) \
        cls->AddProperty(eoa::MakeProperty<Classname, decltype(Member)>( \
            #Member, Type, &Classname::Member));

// Свойство с метаданными
#define EOA_PROPERTY_META(Member, Type, ...) \
        { \
            eoa::PropertyMetadata meta; \
            __VA_ARGS__; \
            cls->AddProperty(eoa::MakeProperty<Classname, decltype(Member)>( \
                #Member, Type, &Classname::Member, meta)); \
        }

// Свойство только для чтения (getter)
#define EOA_PROPERTY_READONLY(Getter, Type) \
        cls->AddProperty(eoa::MakeReadOnlyProperty<Classname, decltype(Classname::Getter())>( \
            #Getter, Type, &Classname::Getter));

// Свойство с getter/setter функциями
#define EOA_PROPERTY_FUNC(Getter, Setter, Type) \
        cls->AddProperty(eoa::MakeProperty<Classname, decltype(Getter())>( \
            #Getter, Type, &Classname::Getter, &Classname::Setter));

// Функция/метод класса
#define EOA_FUNCTION(Method) \
        cls->AddFunction(eoa::MakeFunction<Classname, decltype(&Classname::Method)>( \
            #Method, &Classname::Method));

// Функция с метаданными
#define EOA_FUNCTION_META(Method, ...) \
        { \
            eoa::FunctionMetadata meta; \
            __VA_ARGS__; \
            cls->AddFunction(eoa::MakeFunction<Classname, decltype(&Classname::Method)>( \
                #Method, &Classname::Method, meta)); \
        }

#define EOA_END_CLASS_IMPL() \
        eoa::ReflectionSystem::Get().RegisterClass(std::move(cls)); \
    }

// ============================================================================
// Макрос для автоматической регистрации класса при загрузке
// ============================================================================
#define EOA_REGISTER_CLASS(Classname) \
    namespace eoa { namespace detail { \
        struct Classname##Registrar { \
            Classname##Registrar() { \
                Classname::RegisterReflection(); \
            } \
        }; \
        static Classname##Registrar g_##Classname##Registrar; \
    }}

// ============================================================================
// Макросы для регистрации enum
// ============================================================================
#define EOA_ENUM_DECL(EnumName) \
    void RegisterEnum##EnumName() { \
        auto enm = eoa::MakeEnum<EnumName>(#EnumName);

#define EOA_ENUM_VALUE(Value) \
        enm->AddValue(#Value, static_cast<int64_t>(Value));

#define EOA_ENUM_END() \
        eoa::ReflectionSystem::Get().RegisterEnum(std::move(enm)); \
    } \
    EOA_REGISTER_ENUM_HELPER(EnumName)

#define EOA_REGISTER_ENUM_HELPER(EnumName) \
    namespace eoa { namespace detail { \
        struct EnumName##Registrar { \
            EnumName##Registrar() { \
                RegisterEnum##EnumName(); \
            } \
        }; \
        static EnumName##Registrar g_##EnumName##Registrar; \
    }}

// ============================================================================
// Helper макросы для метаданных
// ============================================================================
#define META_DISPLAY_NAME(name) meta.displayName = name
#define META_CATEGORY(cat) meta.category = cat
#define META_TOOLTIP(tip) meta.tooltip = tip
#define META_RANGE(min, max) meta.minValue = min; meta.maxValue = max
#define META_STEP(s) meta.step = s
#define META_READ_ONLY meta.readOnly = true
#define META_VISIBLE(vis) meta.visible = vis
#define META_ADVANCED meta.advanced = true
#define META_GROUP(grp) meta.group = grp
#define META_SORT_ORDER(order) meta.sortOrder = order
#define META_COLOR meta.isColor = true
#define META_COLOR_NO_ALPHA meta.isColor = true; meta.hasAlpha = false
#define META_FILE_PATH(filter) meta.isFilePath = true; meta.fileFilter = filter
#define META_MULTILINE(rows) meta.multiline = true; meta.multilineRows = rows
#define META_ENUM_TYPE(type) meta.enumType = type
