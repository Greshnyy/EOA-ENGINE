#include "core/object.h"

namespace eoa {

uint64_t Object::s_NextInstanceID = 1;

Object::Object()
    : name_("Unnamed")
    , instanceID_(s_NextInstanceID++)
{
}

} // namespace eoa
