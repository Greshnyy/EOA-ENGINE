#pragma once
#include "core/type_info.h"
#include "core/object.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

// Forward declare json library
namespace nlohmann {
    class json;
}

namespace eoa {

// ============================================================================
// Сериализация - базовый класс для сохранения/загрузки объектов
// ============================================================================
class Serializer {
public:
    virtual ~Serializer() = default;
    
    // Сериализовать объект в строку
    virtual std::string Serialize(Object* obj) = 0;
    
    // Десериализовать объект из строки
    virtual Object* Deserialize(const std::string& data, const std::string& className) = 0;
    
    // Сохранить в файл
    bool SaveToFile(Object* obj, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        file << Serialize(obj);
        file.close();
        return true;
    }
    
    // Загрузить из файла
    Object* LoadFromFile(const std::string& filename, const std::string& className) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return nullptr;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        return Deserialize(buffer.str(), className);
    }
};

// ============================================================================
// JSON Сериализатор с использованием рефлексии
// ============================================================================
class JsonSerializer : public Serializer {
public:
    JsonSerializer() = default;
    ~JsonSerializer() override = default;
    
    std::string Serialize(Object* obj) override;
    Object* Deserialize(const std::string& data, const std::string& className) override;
    
private:
    // Вспомогательные функции для конвертации типов в JSON
    void ToJson(nlohmann::json& j, PropertyType type, const std::any& value);
    std::any FromJson(const nlohmann::json& j, PropertyType type, const std::string& typeName);
    
    // Сериализация отдельного свойства
    void SerializeProperty(nlohmann::json& j, Property* prop, void* instance);
    void DeserializeProperty(Property* prop, void* instance, const nlohmann::json& j);
};

// ============================================================================
// Утилиты для работы с путями и именами файлов
// ============================================================================
class SerializationUtils {
public:
    // Получить относительный путь
    static std::string GetRelativePath(const std::string& fullPath, const std::string& basePath);
    
    // Нормализовать путь (конвертировать backslashes в forward slashes)
    static std::string NormalizePath(const std::string& path);
    
    // Проверить существование файла
    static bool FileExists(const std::string& filename);
    
    // Создать директорию если не существует
    static bool CreateDirectoryIfNotExists(const std::string& path);
    
    // Получить расширение файла
    static std::string GetFileExtension(const std::string& filename);
    
    // Получить имя файла без расширения
    static std::string GetFileNameWithoutExtension(const std::string& filename);
};

} // namespace eoa
