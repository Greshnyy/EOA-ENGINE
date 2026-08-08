#pragma once

#include "Core/Platform.h"
#include <cstdint>
#include <cstddef>

namespace EOA {

// Базовые типы
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

// ID типов
using EntityID = uint32;
using ComponentID = uint32;
using ResourceID = uint32;

// Константы
constexpr uint32 INVALID_ID = 0xFFFFFFFF;
constexpr float PI = 3.14159265358979323846f;
constexpr float EPSILON = 0.0001f;

} // namespace EOA
