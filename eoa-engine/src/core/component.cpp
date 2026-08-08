#include "core/component.h"
#include "core/actor.h"

namespace eoa {

Component::Component(const std::string& name) {
    SetName(name);
}

// Регистрация класса Component
EOA_CLASS_IMPL(Component, Object)
EOA_END_CLASS_IMPL()

} // namespace eoa
