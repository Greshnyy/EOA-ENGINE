#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // Circular point sprite
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float d = dot(uv, uv);
    if (d > 1.0) discard;
    float alpha = 1.0 - smoothstep(0.0, 1.0, d);
    outColor = fragColor * vec4(1,1,1, alpha);
}
