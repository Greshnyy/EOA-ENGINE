#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 lightDir;
    vec4 lightColor;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-ubo.lightDir.xyz);
    float NdotL = max(dot(N, L), 0.0);

    vec3 ambient = vec3(0.1) * ubo.lightColor.rgb;
    vec3 diffuse = NdotL * ubo.lightColor.rgb * ubo.lightColor.w;

    vec4 texColor = texture(texSampler, fragUV);
    vec3 lit = (ambient + diffuse) * fragColor * texColor.rgb;

    outColor = vec4(lit, texColor.a);
}
