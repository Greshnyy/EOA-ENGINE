#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

// Combined push constants: model (vert) + material params (frag)
// Total: 64 (mat4) + 16 (vec4) + 4 + 4 + 8 (pad) = 96 bytes
layout(push_constant) uniform PushConstants {
    mat4 model;
    layout(offset = 64) vec4 baseColorFactor;
    layout(offset = 80) float roughnessFactor;
    layout(offset = 84) float metallicFactor;
} pc;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 lightColor;
} camera;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragColor;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = camera.proj * camera.view * worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = mat3(pc.model) * inNormal;
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}
