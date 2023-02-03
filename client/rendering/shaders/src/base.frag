#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 fragColor;

layout(set=1, binding=1) uniform sampler2D textures[];

void main() {
    outColor = fragColor;
}