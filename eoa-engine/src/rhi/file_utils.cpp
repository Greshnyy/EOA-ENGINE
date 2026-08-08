#include "rhi/file_utils.h"
#include "log.h"
#include <fstream>

namespace eoa {

std::vector<char> ReadFileBinary(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        EOA_FATAL("Не удалось открыть файл: %s "
                  "(запускаешь exe не из build/? shaders/ ищется относительно рабочей директории)",
                  path.c_str());
    }
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));
    return buffer;
}

} // namespace eoa
