#include "core/serializer.h"
#include "third_party/json.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <filesystem>

namespace eoa {

namespace fs = std::filesystem;
using json = nlohmann::json;

// ============================================================================
// JsonSerializer implementation
// ============================================================================

void JsonSerializer::ToJson(json& j, PropertyType type, const std::any& value) {
    if (!value.has_value()) {
        j = nullptr;
        return;
    }
    
    switch (type) {
        case PropertyType::Bool:
            j = std::any_cast<bool>(value);
            break;
            
        case PropertyType::Int:
        case PropertyType::Int8:
        case PropertyType::Int16:
        case PropertyType::Int64:
            j = std::any_cast<int>(value);
            break;
            
        case PropertyType::UInt:
        case PropertyType::UInt8:
        case PropertyType::UInt16:
        case PropertyType::UInt64:
            j = std::any_cast<unsigned int>(value);
            break;
            
        case PropertyType::Float:
            j = std::any_cast<float>(value);
            break;
            
        case PropertyType::Double:
            j = std::any_cast<double>(value);
            break;
            
        case PropertyType::String:
            j = std::any_cast<std::string>(value);
            break;
            
        case PropertyType::Vec2: {
            auto v = std::any_cast<glm::vec2>(value);
            j = {v.x, v.y};
            break;
        }
        
        case PropertyType::Vec3: {
            auto v = std::any_cast<glm::vec3>(value);
            j = {v.x, v.y, v.z};
            break;
        }
        
        case PropertyType::Vec4: {
            auto v = std::any_cast<glm::vec4>(value);
            j = {v.x, v.y, v.z, v.w};
            break;
        }
        
        case PropertyType::Quat: {
            auto q = std::any_cast<glm::quat>(value);
            j = {q.x, q.y, q.z, q.w};
            break;
        }
        
        case PropertyType::Mat2: {
            auto m = std::any_cast<glm::mat2>(value);
            j = {
                {m[0][0], m[0][1]},
                {m[1][0], m[1][1]}
            };
            break;
        }
        
        case PropertyType::Mat3: {
            auto m = std::any_cast<glm::mat3>(value);
            j = {
                {m[0][0], m[0][1], m[0][2]},
                {m[1][0], m[1][1], m[1][2]},
                {m[2][0], m[2][1], m[2][2]}
            };
            break;
        }
        
        case PropertyType::Mat4: {
            auto m = std::any_cast<glm::mat4>(value);
            j = {
                {m[0][0], m[0][1], m[0][2], m[0][3]},
                {m[1][0], m[1][1], m[1][2], m[1][3]},
                {m[2][0], m[2][1], m[2][2], m[2][3]},
                {m[3][0], m[3][1], m[3][2], m[3][3]}
            };
            break;
        }
        
        case PropertyType::Object: {
            auto obj = std::any_cast<Object*>(value);
            if (obj) {
                j = {
                    {"__type__", obj->ClassName()},
                    {"__id__", reinterpret_cast<uintptr_t>(obj)}
                };
            } else {
                j = nullptr;
            }
            break;
        }
        
        case PropertyType::Enum: {
            // Enum сохраняем как строку с именем значения
            auto val = std::any_cast<int64_t>(value);
            j = val;
            break;
        }
        
        default:
            // Для неподдерживаемых типов пытаемся сохранить как строку
            try {
                j = std::any_cast<std::string>(value);
            } catch (...) {
                j = nullptr;
            }
            break;
    }
}

std::any JsonSerializer::FromJson(const json& j, PropertyType type, const std::string& typeName) {
    if (j.is_null()) {
        return std::any();
    }
    
    switch (type) {
        case PropertyType::Bool:
            return std::any(j.get<bool>());
            
        case PropertyType::Int:
        case PropertyType::Int8:
        case PropertyType::Int16:
        case PropertyType::Int64:
            return std::any(j.get<int>());
            
        case PropertyType::UInt:
        case PropertyType::UInt8:
        case PropertyType::UInt16:
        case PropertyType::UInt64:
            return std::any(j.get<unsigned int>());
            
        case PropertyType::Float:
            return std::any(j.get<float>());
            
        case PropertyType::Double:
            return std::any(j.get<double>());
            
        case PropertyType::String:
            return std::any(j.get<std::string>());
            
        case PropertyType::Vec2: {
            auto arr = j.get<std::vector<float>>();
            if (arr.size() >= 2) {
                return std::any(glm::vec2(arr[0], arr[1]));
            }
            return std::any(glm::vec2(0.0f));
        }
        
        case PropertyType::Vec3: {
            auto arr = j.get<std::vector<float>>();
            if (arr.size() >= 3) {
                return std::any(glm::vec3(arr[0], arr[1], arr[2]));
            }
            return std::any(glm::vec3(0.0f));
        }
        
        case PropertyType::Vec4: {
            auto arr = j.get<std::vector<float>>();
            if (arr.size() >= 4) {
                return std::any(glm::vec4(arr[0], arr[1], arr[2], arr[3]));
            }
            return std::any(glm::vec4(0.0f));
        }
        
        case PropertyType::Quat: {
            auto arr = j.get<std::vector<float>>();
            if (arr.size() >= 4) {
                return std::any(glm::quat(arr[0], arr[1], arr[2], arr[3]));
            }
            return std::any(glm::quat(0.0f, 0.0f, 0.0f, 1.0f));
        }
        
        case PropertyType::Mat2: {
            auto arr = j.get<std::vector<std::vector<float>>>();
            if (arr.size() >= 2 && arr[0].size() >= 2) {
                glm::mat2 m;
                m[0][0] = arr[0][0]; m[0][1] = arr[0][1];
                m[1][0] = arr[1][0]; m[1][1] = arr[1][1];
                return std::any(m);
            }
            return std::any(glm::mat2(1.0f));
        }
        
        case PropertyType::Mat3: {
            auto arr = j.get<std::vector<std::vector<float>>>();
            if (arr.size() >= 3 && arr[0].size() >= 3) {
                glm::mat3 m;
                for (int i = 0; i < 3; ++i)
                    for (int k = 0; k < 3; ++k)
                        m[i][k] = arr[i][k];
                return std::any(m);
            }
            return std::any(glm::mat3(1.0f));
        }
        
        case PropertyType::Mat4: {
            auto arr = j.get<std::vector<std::vector<float>>>();
            if (arr.size() >= 4 && arr[0].size() >= 4) {
                glm::mat4 m;
                for (int i = 0; i < 4; ++i)
                    for (int k = 0; k < 4; ++k)
                        m[i][k] = arr[i][k];
                return std::any(m);
            }
            return std::any(glm::mat4(1.0f));
        }
        
        case PropertyType::Enum:
            return std::any(j.get<int64_t>());
            
        default:
            return std::any();
    }
}

void JsonSerializer::SerializeProperty(json& j, Property* prop, void* instance) {
    if (!prop || !instance) return;
    
    auto value = prop->Get(instance);
    ToJson(j, prop->GetType(), value);
}

void JsonSerializer::DeserializeProperty(Property* prop, void* instance, const json& j) {
    if (!prop || !instance) return;
    
    auto value = FromJson(j, prop->GetType(), prop->GetTypeName());
    prop->Set(instance, value);
}

std::string JsonSerializer::Serialize(Object* obj) {
    if (!obj) return "{}";
    
    json root;
    auto className = obj->ClassName();
    auto cls = ReflectionSystem::Get().GetClass(className);
    
    if (!cls) {
        return "{}";
    }
    
    // Метаданные
    root["__type__"] = className;
    root["__version__"] = 1;
    
    // Сериализация всех свойств
    json properties;
    for (const auto& [name, prop] : cls->GetProperties()) {
        json propJson;
        SerializeProperty(propJson, prop.get(), obj);
        properties[name] = propJson;
    }
    root["properties"] = properties;
    
    return root.dump(4);  // Красивый вывод с отступами в 4 пробела
}

Object* JsonSerializer::Deserialize(const std::string& data, const std::string& className) {
    try {
        json root = json::parse(data);
        
        // Проверка типа
        if (root.contains("__type__")) {
            std::string type = root["__type__"];
            if (type != className) {
                // Тип не совпадает, можно попробовать конвертацию или выдать ошибку
                LOG_WARN("Type mismatch: expected {}, got {}", className, type);
            }
        }
        
        // Создание экземпляра
        auto obj = ReflectionSystem::Get().CreateObject(className);
        if (!obj) {
            LOG_ERROR("Failed to create object of type {}", className);
            return nullptr;
        }
        
        // Десериализация свойств
        if (root.contains("properties")) {
            auto cls = ReflectionSystem::Get().GetClass(className);
            if (cls) {
                for (auto& [name, propJson] : root["properties"].items()) {
                    auto prop = cls->GetProperty(name);
                    if (prop) {
                        DeserializeProperty(prop, obj.get(), propJson);
                    }
                }
            }
        }
        
        return obj.release();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Deserialization error: {}", e.what());
        return nullptr;
    }
}

// ============================================================================
// SerializationUtils implementation
// ============================================================================

std::string SerializationUtils::GetRelativePath(const std::string& fullPath, const std::string& basePath) {
    try {
        fs::path full(fullPath);
        fs::path base(basePath);
        return fs::relative(full, base.parent_path()).string();
    } catch (...) {
        return fullPath;
    }
}

std::string SerializationUtils::NormalizePath(const std::string& path) {
    std::string result = path;
    for (char& c : result) {
        if (c == '\\') c = '/';
    }
    return result;
}

bool SerializationUtils::FileExists(const std::string& filename) {
    return fs::exists(filename);
}

bool SerializationUtils::CreateDirectoryIfNotExists(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (...) {
        return false;
    }
}

std::string SerializationUtils::GetFileExtension(const std::string& filename) {
    try {
        return fs::path(filename).extension().string();
    } catch (...) {
        return "";
    }
}

std::string SerializationUtils::GetFileNameWithoutExtension(const std::string& filename) {
    try {
        return fs::path(filename).stem().string();
    } catch (...) {
        return filename;
    }
}

} // namespace eoa
