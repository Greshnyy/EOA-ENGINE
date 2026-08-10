#pragma once
#include "core/type_info.h"
#include "core/object.h"
#include "json.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace eoa {

class Serializer {
public:
    virtual ~Serializer() = default;
    virtual std::string Serialize(Object* obj) = 0;
    virtual Object* Deserialize(const std::string& data, const std::string& className) = 0;

    bool SaveToFile(Object* obj, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        file << Serialize(obj);
        return true;
    }

    Object* LoadFromFile(const std::string& filename, const std::string& className) {
        std::ifstream file(filename);
        if (!file.is_open()) return nullptr;
        std::stringstream buffer;
        buffer << file.rdbuf();
        return Deserialize(buffer.str(), className);
    }
};

class JsonSerializer : public Serializer {
public:
    JsonSerializer() = default;
    ~JsonSerializer() override = default;

    std::string Serialize(Object* obj) override;
    Object* Deserialize(const std::string& data, const std::string& className) override;

private:
    void ToJson(nlohmann::json& j, PropertyType type, const std::any& value);
    std::any FromJson(const nlohmann::json& j, PropertyType type, const std::string& typeName);
    void SerializeProperty(nlohmann::json& j, Property* prop, void* instance);
    void DeserializeProperty(Property* prop, void* instance, const nlohmann::json& j);
};

class SerializationUtils {
public:
    static std::string GetRelativePath(const std::string& fullPath, const std::string& basePath);
    static std::string NormalizePath(const std::string& path);
    static bool FileExists(const std::string& filename);
    static bool CreateDirectoryIfNotExists(const std::string& path);
    static std::string GetFileExtension(const std::string& filename);
    static std::string GetFileNameWithoutExtension(const std::string& filename);
};

} // namespace eoa
