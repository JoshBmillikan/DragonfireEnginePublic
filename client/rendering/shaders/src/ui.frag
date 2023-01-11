#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 uv;
layout(binding = 0) uniform sampler2D textureSampler;

void main() {
    outColor = texture(textureSampler, uv);
}