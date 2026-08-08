#pragma once

#include "core/eoa_core.h"
#include "core/type_info.h"
#include "core/reflection_macros.h"

namespace EOA
{
    // Ре-экспорт классов рефлексии для удобства
    using Reflectable = IReflectable;
    using ReflectionSystem = GlobalReflectionSystem;
}
