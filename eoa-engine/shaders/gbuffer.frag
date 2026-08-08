#version 460

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragColor;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outORM;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
    layout(offset = 64) vec4 baseColorFactor;
    layout(offset = 80) float roughnessFactor;
    layout(offset = 84) float metallicFactor;
} pc;

void main() {
    vec3 N = normalize(fragNormal);

    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 albedo = fragColor * pc.baseColorFactor.rgb * texColor.rgb;

    float roughness = pc.roughnessFactor;
    float metallic = pc.metallicFactor;

    outAlbedo = vec4(albedo, 1.0);
    outNormal = vec4(N * 0.5 + 0.5, 0.0);
    outORM = vec4(roughness, metallic, 0.0, 0.0);
}
