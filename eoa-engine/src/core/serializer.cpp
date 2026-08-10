#include "core/serializer.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "log.h"

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
            j = {{m[0][0], m[0][1]}, {m[1][0], m[1][1]}};
            break;
        }
        case PropertyType::Mat3: {
            auto m = std::any_cast<glm::mat3>(value);
            j = {{m[0][0], m[0][1], m[0][2]},
                 {m[1][0], m[1][1], m[1][2]},
                 {m[2][0], m[2][1], m[2][2]}};
            break;
        }
        case PropertyType::Mat4: {
            auto m = std::any_cast<glm::mat4>(value);
            j = {{m[0][0], m[0][1], m[0][2], m[0][3]},
                 {m[1][0], m[1][1], m[1][2], m[1][3]},
                 {m[2][0], m[2][1], m[2][2], m[2][3]},
                 {m[3][0], m[3][1], m[3][2], m[3][3]}};
            break;
        }
        case PropertyType::Object: {
            auto obj = std::any_cast<Object*>(value);
            if (obj) {
                j = {{"__type__", obj->ClassName()},
                     {"__id__", reinterpret_cast<uintptr_t>(obj)}};
            } else {
                j = nullptr;
            }
            break;
        }
        case PropertyType::Enum:
            j = std::any_cast<int64_t>(value);
            break;
        default:
            try {
                j = std::any_cast<std::string>(value);
            } catch (...) {
                j = nullptr;
            }
            break;
    }
}

std::any JsonSerializer::FromJson(const json& j, PropertyType type, const std::string&) {
    if (j.is_null()) return std::any();

    switch (type) {
        case PropertyType::Bool: return std::any(j.get<bool>());
        case PropertyType::Int:
        case PropertyType::Int8:
        case PropertyType::Int16:
        case PropertyType::Int64: return std::any(j.get<int>());
        case PropertyType::UInt:
        case PropertyType::UInt8:
        case PropertyType::UInt16:
        case PropertyType::UInt64: return std::any(j.get<unsigned int>());
        case PropertyType::Float: return std::any(j.get<float>());
        case PropertyType::Double: return std::any(j.get<double>());
        case PropertyType::String: return std::any(j.get<std::string>());
        case PropertyType::Vec2: {
            auto a = j.get<std::vector<float>>();
            return std::any(a.size() >= 2 ? glm::vec2(a[0], a[1]) : glm::vec2(0.0f));
        }
        case PropertyType::Vec3: {
            auto a = j.get<std::vector<float>>();
            return std::any(a.size() >= 3 ? glm::vec3(a[0], a[1], a[2]) : glm::vec3(0.0f));
        }
        case PropertyType::Vec4: {
            auto a = j.get<std::vector<float>>();
            return std::any(a.size() >= 4 ? glm::vec4(a[0], a[1], a[2], a[3]) : glm::vec4(0.0f));
        }
        case PropertyType::Quat: {
            auto a = j.get<std::vector<float>>();
            return std::any(a.size() >= 4 ? glm::quat(a[3], a[0], a[1], a[2]) : glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        }
        case PropertyType::Mat2: {
            auto a = j.get<std::vector<std::vector<float>>>();
            if (a.size() >= 2 && a[0].size() >= 2 && a[1].size() >= 2) {
                glm::mat2 m;
                for (int c = 0; c < 2; ++c) for (int r = 0; r < 2; ++r) m[c][r] = a[c][r];
                return std::any(m);
            }
            return std::any(glm::mat2(1.0f));
        }
        case PropertyType::Mat3: {
            auto a = j.get<std::vector<std::vector<float>>>();
            if (a.size() >= 3 && a[0].size() >= 3 && a[1].size() >= 3 && a[2].size() >= 3) {
                glm::mat3 m;
                for (int c = 0; c < 3; ++c) for (int r = 0; r < 3; ++r) m[c][r] = a[c][r];
                return std::any(m);
            }
            return std::any(glm::mat3(1.0f));
        }
        case PropertyType::Mat4: {
            auto a = j.get<std::vector<std::vector<float>>>();
            if (a.size() >= 4 && a[0].size() >= 4 && a[1].size() >= 4 && a[2].size() >= 4 && a[3].size() >= 4) {
                glm::mat4 m;
                for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) m[c][r] = a[c][r];
                return std::any(m);
            }
            return std::any(glm::mat4(1.0f));
        }
        case PropertyType::Enum: return std::any(j.get<int64_t>());
        default: return std::any();
    }
}

void JsonSerializer::SerializeProperty(json& j, Property* prop, void* instance) {
    if (!prop || !instance) return;
    ToJson(j, prop->GetType(), prop->Get(instance));
}

void JsonSerializer::DeserializeProperty(Property* prop, void* instance, const json& j) {
    if (!prop || !instance) return;
    prop->Set(instance, FromJson(j, prop->GetType(), prop->GetTypeName()));
}

std::string JsonSerializer::Serialize(Object* obj) {
    if (!obj) return "{}";
    json root;
    const auto className = obj->ClassName();
    const auto cls = ReflectionSystem::Get().GetClass(className);
    if (!cls) return "{}";

    root["__type__"] = className;
    root["__version__"] = 1;
    json properties;
    for (const auto& [name, prop] : cls->GetProperties()) {
        json propJson;
        SerializeProperty(propJson, prop.get(), obj);
        properties[name] = propJson;
    }
    root["properties"] = properties;
    return root.dump(4);
}

Object* JsonSerializer::Deserialize(const std::string& data, const std::string& className) {
    try {
        json root = json::parse(data);
        if (root.contains("__type__")) {
            const std::string type = root["__type__"];
            if (type != className) {
                EOA_WARN("Type mismatch: expected '%s', got '%s'", className.c_str(), type.c_str());
            }
        }

        auto obj = ReflectionSystem::Get().CreateObject(className);
        if (!obj) {
            EOA_ERROR("Failed to create object of type '%s'", className.c_str());
            return nullptr;
        }

        if (root.contains("properties")) {
            auto cls = ReflectionSystem::Get().GetClass(className);
            if (cls) {
                for (auto& [name, propJson] : root["properties"].items()) {
                    auto prop = cls->GetProperty(name);
                    if (prop) DeserializeProperty(prop, obj.get(), propJson);
                }
            }
        }
        return obj.release();
    } catch (const std::exception& e) {
        EOA_ERROR("Deserialization error: %s", e.what());
        return nullptr;
    }
}

std::string SerializationUtils::GetRelativePath(const std::string& fullPath, const std::string& basePath) {
    try {
        return fs::relative(fs::path(fullPath), fs::path(basePath).parent_path()).string();
    } catch (...) {
        return fullPath;
    }
}

std::string SerializationUtils::NormalizePath(const std::string& path) {
    std::string result = path;
    for (char& c : result) if (c == '\\') c = '/';
    return result;
}

bool SerializationUtils::FileExists(const std::string& filename) { return fs::exists(filename); }

bool SerializationUtils::CreateDirectoryIfNotExists(const std::string& path) {
    try { return fs::create_directories(path); } catch (...) { return false; }
}

std::string SerializationUtils::GetFileExtension(const std::string& filename) {
    try { return fs::path(filename).extension().string(); } catch (...) { return {}; }
}

std::string SerializationUtils::GetFileNameWithoutExtension(const std::string& filename) {
    try { return fs::path(filename).stem().string(); } catch (...) { return filename; }
}

} // namespace eoa
