#version 460

layout(location = 0) out vec4 outColor;

void main() {
    // Depth-only pass — output is handled by depth attachment
    // Fragment shader required by Vulkan but doesn't need to write color
    outColor = vec4(1.0);
}
