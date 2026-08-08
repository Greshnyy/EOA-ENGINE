#version 450
layout(location = 0) in vec3 inPosition;

struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

layout(binding = 1) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec4 fragColor;

void main() {
    Particle p = particles[gl_VertexIndex];
    if (p.position.w <= 0.0) {
        gl_Position = vec4(0, 0, 0, 0);
        return;
    }
    vec4 worldPos = vec4(p.position.xyz, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    gl_PointSize = p.position.w * 50.0 / gl_Position.w;
    fragColor = p.color;
}
