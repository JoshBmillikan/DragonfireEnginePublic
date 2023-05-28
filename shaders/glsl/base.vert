#version 460

layout (location=0) in vec3 position;
layout (location=1) in vec3 normal;
layout (location=2) in vec2 uv;

layout (set=0, binding=0) uniform Ubo {
    mat4 perspective;
    mat4 orthographic;
    vec3 sunDirection;
    vec2 resolution;
}uboData;

layout (std430, set=1, binding=0) readonly buffer Transforms {
    mat4 modelMatrices[];
}transforms;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 uv_out;

void main()
{
    mat4 transform = uboData.perspective * transforms.modelMatrices[gl_InstanceIndex];
    gl_Position = transform * vec4(position, 1.0);

    vec4 ambient = vec4(0.75, 0.75, 0.75, 1.0);
    vec4 diffuse = vec4(max(dot(vec3(0.24525, -0.919709, 0.30656966), -normal), 0) * vec3(1.0, 1.0, 1.0), 1);
    frag_color = ambient + diffuse;
    uv_out = uv;
}