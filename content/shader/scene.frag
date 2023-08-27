#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_texcoord;
#ifdef MOTION_VECTORS
layout(location = 2) in vec4 screen_position;
layout(location = 3) in vec4 prev_screen_position;
#endif
layout(location = 0) out vec4 out_color;
#ifdef MOTION_VECTORS
layout(location = 1) out vec2 motion_vector;
#endif

struct ViewData
{
    mat4 view_matrix;
    mat4 proj_matrix;
    vec4 view_position;
    vec2 taa_jitter;
};

layout(std140, set = 0, binding = 1) uniform ViewBuffer
{
    ViewData cur;
    ViewData prev;
} view_buffer;

void main()
{
    vec3 sun_direction = normalize(vec3(0.0, -1.0, -1.0));
    out_color = vec4(max(dot(normalize(in_normal), -sun_direction), 0.0) * vec3(0.6) + vec3(0.1), 1.0);
    #ifdef MOTION_VECTORS
    vec2 position_clip = (screen_position.xy / screen_position.w) - view_buffer.cur.taa_jitter;
    vec2 prev_position_clip = (prev_screen_position.xy / prev_screen_position.w) - view_buffer.prev.taa_jitter;

    vec2 position_uv = position_clip * 0.5 + 0.5;
    vec2 prev_position_uv = prev_position_clip * 0.5 + 0.5;

    motion_vector = position_uv - prev_position_uv;
    #endif
}