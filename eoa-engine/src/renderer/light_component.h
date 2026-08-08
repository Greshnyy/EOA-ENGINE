#pragma once
#include "core/component.h"
#include <glm/glm.hpp>

namespace eoa {

enum class LightType : uint8_t {
    Directional = 0,
    Point = 1,
    Spot = 2
};

class LightComponent : public Component {
public:
    const char* ClassName() const override { return "LightComponent"; }

    explicit LightComponent(const std::string& name = "Light");

    LightType GetLightType() const { return type_; }
    void SetLightType(LightType type) { type_ = type; }

    const glm::vec3& GetColor() const { return color_; }
    void SetColor(const glm::vec3& color) { color_ = color; }

    float GetIntensity() const { return intensity_; }
    void SetIntensity(float intensity) { intensity_ = intensity; }

    // Point/Spot
    float GetRange() const { return range_; }
    void SetRange(float range) { range_ = range; }

    // Spot
    float GetInnerConeAngle() const { return innerConeAngle_; }
    void SetInnerConeAngle(float deg) { innerConeAngle_ = deg; }
    float GetOuterConeAngle() const { return outerConeAngle_; }
    void SetOuterConeAngle(float deg) { outerConeAngle_ = deg; }

    // Для передачи в шейдер (32 байта, выровнено как vec4)
    struct ShaderData {
        glm::vec3 color;      // 12
        float intensity;       // 4
        glm::vec3 position;    // 12
        float range;           // 4
        glm::vec3 direction;   // 12
        uint32_t type;         // 4
        float innerCone;       // 4
        float outerCone;       // 4
        float _pad[2];         // 8 — до выравнивания 16
        // Total: 60 → округляем до 64 (кратно 16)
    };

    void FillShaderData(ShaderData& data) const;

private:
    LightType type_ = LightType::Directional;
    glm::vec3 color_ = glm::vec3(1.0f);
    float intensity_ = 1.0f;
    float range_ = 10.0f;
    float innerConeAngle_ = 12.5f;
    float outerConeAngle_ = 20.0f;
};

} // namespace eoa
