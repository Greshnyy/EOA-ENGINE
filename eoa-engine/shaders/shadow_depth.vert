#version 460

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform LightSpaceUBO {
    mat4 lightViewProj;
} lightSpace;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = lightSpace.lightViewProj * pc.model * vec4(inPosition, 1.0);
}
