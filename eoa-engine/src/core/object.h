#pragma once
#include <string>
#include <cstdint>

namespace eoa {

class Object {
public:
    virtual ~Object() = default;
    virtual const char* ClassName() const = 0;

    const std::string& GetName() const { return name_; }
    void SetName(const std::string& name) { name_ = name; }
    uint64_t GetInstanceID() const { return instanceID_; }

protected:
    Object();

private:
    std::string name_;
    uint64_t instanceID_;
    static uint64_t s_NextInstanceID;
};


} // namespace eoa
