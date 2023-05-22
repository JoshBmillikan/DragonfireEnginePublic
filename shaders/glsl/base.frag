#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 uv;

//layout(set=1, binding=0) uniform sampler2D textures[];

//layout(push_constant) uniform textureIndices
//{
//    uint albedo;
//    uint roughness;
//    uint emissive;
//    uint normal;
//};

void main() {
    //outColor = texture(textures[albedo], uv);
    outColor = fragColor;
}