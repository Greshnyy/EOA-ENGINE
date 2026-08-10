#include "core/serializer.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <filesystem>
#include "json.hpp"

namespace eoa {
namespace fs = std::filesystem;
using json = nlohmann::json;

void JsonSerializer::ToJson(json& j, PropertyType type, const std::any& value) {
    if (!value.has_value()) { j = nullptr; return; }
    switch (type) {
        case PropertyType::Bool: j = std::any_cast<bool>(value); break;
        case PropertyType::Int:
        case PropertyType::Int8:
        case PropertyType::Int16:
        case PropertyType::Int64: j = std::any_cast<int>(value); break;
        case PropertyType::UInt:
        case PropertyType::UInt8:
        case PropertyType::UInt16:
        case PropertyType::UInt64: j = std::any_cast<unsigned int>(value); break;
        case PropertyType::Float: j = std::any_cast<float>(value); break;
        case PropertyType::Double: j = std::any_cast<double>(value); break;
        case PropertyType::String: j = std::any_cast<std::string>(value); break;
        case PropertyType::Vec2: { auto v=std::any_cast<glm::vec2>(value); j={v.x,v.y}; break; }
        case PropertyType::Vec3: { auto v=std::any_cast<glm::vec3>(value); j={v.x,v.y,v.z}; break; }
        case PropertyType::Vec4: { auto v=std::any_cast<glm::vec4>(value); j={v.x,v.y,v.z,v.w}; break; }
        case PropertyType::Quat: { auto q=std::any_cast<glm::quat>(value); j={q.x,q.y,q.z,q.w}; break; }
        case PropertyType::Mat2: { auto m=std::any_cast<glm::mat2>(value); j={{m[0][0],m[0][1]},{m[1][0],m[1][1]}}; break; }
        case PropertyType::Mat3: { auto m=std::any_cast<glm::mat3>(value); j={{m[0][0],m[0][1],m[0][2]},{m[1][0],m[1][1],m[1][2]},{m[2][0],m[2][1],m[2][2]}}; break; }
        case PropertyType::Mat4: { auto m=std::any_cast<glm::mat4>(value); j={{m[0][0],m[0][1],m[0][2],m[0][3]},{m[1][0],m[1][1],m[1][2],m[1][3]},{m[2][0],m[2][1],m[2][2],m[2][3]},{m[3][0],m[3][1],m[3][2],m[3][3]}}; break; }
        case PropertyType::Object: { auto obj=std::any_cast<Object*>(value); if(obj) j={{"__type__",obj->ClassName()},{"__id__",reinterpret_cast<uintptr_t>(obj)}}; else j=nullptr; break; }
        case PropertyType::Enum: j=std::any_cast<int64_t>(value); break;
        default: try { j=std::any_cast<std::string>(value); } catch(...) { j=nullptr; } break;
    }
}

std::any JsonSerializer::FromJson(const json& j, PropertyType type, const std::string&) {
    if (j.is_null()) return std::any();
    switch (type) {
        case PropertyType::Bool: return j.get<bool>();
        case PropertyType::Int:
        case PropertyType::Int8:
        case PropertyType::Int16:
        case PropertyType::Int64: return j.get<int>();
        case PropertyType::UInt:
        case PropertyType::UInt8:
        case PropertyType::UInt16:
        case PropertyType::UInt64: return j.get<unsigned int>();
        case PropertyType::Float: return j.get<float>();
        case PropertyType::Double: return j.get<double>();
        case PropertyType::String: return j.get<std::string>();
        case PropertyType::Vec2: { auto a=j.get<std::vector<float>>(); return a.size()>=2?std::any(glm::vec2(a[0],a[1])):std::any(glm::vec2(0.0f)); }
        case PropertyType::Vec3: { auto a=j.get<std::vector<float>>(); return a.size()>=3?std::any(glm::vec3(a[0],a[1],a[2])):std::any(glm::vec3(0.0f)); }
        case PropertyType::Vec4: { auto a=j.get<std::vector<float>>(); return a.size()>=4?std::any(glm::vec4(a[0],a[1],a[2],a[3])):std::any(glm::vec4(0.0f)); }
        case PropertyType::Quat: { auto a=j.get<std::vector<float>>(); return a.size()>=4?std::any(glm::quat(a[3],a[0],a[1],a[2])):std::any(glm::quat(1.0f,0.0f,0.0f,0.0f)); }
        case PropertyType::Mat2: { auto a=j.get<std::vector<std::vector<float>>>(); glm::mat2 m(1.0f); if(a.size()>=2&&a[0].size()>=2)for(int i=0;i<2;++i)for(int k=0;k<2;++k)m[i][k]=a[i][k]; return m; }
        case PropertyType::Mat3: { auto a=j.get<std::vector<std::vector<float>>>(); glm::mat3 m(1.0f); if(a.size()>=3&&a[0].size()>=3)for(int i=0;i<3;++i)for(int k=0;k<3;++k)m[i][k]=a[i][k]; return m; }
        case PropertyType::Mat4: { auto a=j.get<std::vector<std::vector<float>>>(); glm::mat4 m(1.0f); if(a.size()>=4&&a[0].size()>=4)for(int i=0;i<4;++i)for(int k=0;k<4;++k)m[i][k]=a[i][k]; return m; }
        case PropertyType::Enum: return j.get<int64_t>();
        default: return std::any();
    }
}

void JsonSerializer::SerializeProperty(json& j, Property* prop, void* instance) { if(prop&&instance) ToJson(j,prop->GetType(),prop->Get(instance)); }
void JsonSerializer::DeserializeProperty(Property* prop, void* instance, const json& j) { if(prop&&instance) prop->Set(instance,FromJson(j,prop->GetType(),prop->GetTypeName())); }

std::string JsonSerializer::Serialize(Object* obj) {
    if(!obj) return "{}";
    auto cls=ReflectionSystem::Get().GetClass(obj->ClassName());
    if(!cls) return "{}";
    json root; root["__type__"]=obj->ClassName(); root["__version__"]=1;
    for(const auto& [name,prop]:cls->GetProperties()) SerializeProperty(root["properties"][name],prop.get(),obj);
    return root.dump(4);
}

Object* JsonSerializer::Deserialize(const std::string& data,const std::string& className) {
    try {
        json root=json::parse(data);
        auto obj=ReflectionSystem::Get().CreateObject(className);
        if(!obj) return nullptr;
        auto cls=ReflectionSystem::Get().GetClass(className);
        if(cls&&root.contains("properties")) for(auto& [name,value]:root["properties"].items()) if(auto* prop=cls->GetProperty(name)) DeserializeProperty(prop,obj.get(),value);
        return obj.release();
    } catch(...) { return nullptr; }
}

std::string SerializationUtils::GetRelativePath(const std::string& fullPath,const std::string& basePath){try{return fs::relative(fs::path(fullPath),fs::path(basePath).parent_path()).string();}catch(...){return fullPath;}}
std::string SerializationUtils::NormalizePath(const std::string& path){std::string result=path;for(char& c:result)if(c=='\\')c='/';return result;}
bool SerializationUtils::FileExists(const std::string& filename){return fs::exists(filename);}
bool SerializationUtils::CreateDirectoryIfNotExists(const std::string& path){try{return fs::create_directories(path);}catch(...){return false;}}
std::string SerializationUtils::GetFileExtension(const std::string& filename){try{return fs::path(filename).extension().string();}catch(...){return {};}}
std::string SerializationUtils::GetFileNameWithoutExtension(const std::string& filename){try{return fs::path(filename).stem().string();}catch(...){return filename;}}

} // namespace eoa
