#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <any>

// Типы данных (аналог UE)
namespace EOA
{
    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;
    
    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;
    
    using float32 = float;
    using float64 = double;
}

// Макрос для API экспорта (для Windows DLL)
#ifdef _WIN32
    #ifdef EOA_BUILD_DLL
        #define EOA_API __declspec(dllexport)
    #else
        #define EOA_API __declspec(dllimport)
    #endif
#else
    #define EOA_API
#endif

// Включаем glm для векторов/матриц
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace EOA
{
    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;
    using Quat = glm::quat;
}
