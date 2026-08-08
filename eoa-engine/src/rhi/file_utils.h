#pragma once
#include <vector>
#include <string>

namespace eoa {

// Читает файл целиком в байты. Используется для загрузки скомпилированного
// SPIR-V (.spv). Падает с понятным сообщением, если файла нет — частая
// причина: запустили exe не из build/ (шейдеры лежат рядом относительным путём).
std::vector<char> ReadFileBinary(const std::string& path);

} // namespace eoa
