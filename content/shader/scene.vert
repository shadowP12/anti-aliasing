#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;
layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texcoord;

layout(std140, set = 0, binding = 0) uniform SceneBuffer
{
    mat4 transform;
} scene_buffer;

layout(std140, set = 0, binding = 1) uniform ViewBuffer
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
} view_buffer;

void main() 
{
    gl_Position = view_buffer.proj_matrix * view_buffer.view_matrix * scene_buffer.transform * vec4(in_position, 1.0);
    out_texcoord = in_texcoord;
    out_normal = mat3(transpose(inverse(scene_buffer.transform))) * in_normal;
}