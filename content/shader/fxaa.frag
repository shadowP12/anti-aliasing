#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 in_texcoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform texture2D color_texture;
layout(binding = 1) uniform sampler color_sampler;

#define FXAA_REDUCE_MIN   (1.0/ 128.0)
#define FXAA_REDUCE_MUL   (1.0 / 8.0)
#define FXAA_SPAN_MAX     8.0

vec4 do_fxaa()
{
    vec2 fragCoord = in_texcoord;
    vec4 color = vec4(0.0);
    vec2 size = vec2(textureSize(sampler2D(color_texture, color_sampler), 0));
    vec2 inverseVP = vec2(1.0) / size;
    vec3 rgbNW = texture(sampler2D(color_texture, color_sampler), fragCoord + (vec2(-0.5, -0.5) * inverseVP)).xyz;
    vec3 rgbNE = texture(sampler2D(color_texture, color_sampler), fragCoord + (vec2(0.5, -0.5) * inverseVP)).xyz;
    vec3 rgbSW = texture(sampler2D(color_texture, color_sampler), fragCoord + (vec2(-0.5, 0.5) * inverseVP)).xyz;
    vec3 rgbSE = texture(sampler2D(color_texture, color_sampler), fragCoord + (vec2(0.5, 0.5) * inverseVP)).xyz;
    vec3 rgbM  = texture(sampler2D(color_texture, color_sampler), fragCoord).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * inverseVP;

    vec3 rgbA = 0.5 * (
        texture(sampler2D(color_texture, color_sampler), fragCoord + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture(sampler2D(color_texture, color_sampler), fragCoord + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(sampler2D(color_texture, color_sampler), fragCoord + dir * -0.5).xyz +
        texture(sampler2D(color_texture, color_sampler), fragCoord + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = vec4(rgbA, 1.0);
    else
        color = vec4(rgbB, 1.0);
    return color;
}

void main()
{
    out_color = do_fxaa();
}