#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texcoord;
layout(location = 0) out vec4 out_color;

layout(std140, set = 0, binding = 1) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
} view_buffer;

void main()
{
    vec3 sun_direction = normalize(vec3(0.0, -1.0, -1.0));
    out_color = vec4(max(dot(normalize(in_normal), -sun_direction), 0.0) * vec3(0.6) + vec3(0.1), 1.0);
}