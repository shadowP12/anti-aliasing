#include "fxaa_pass.h"
#include "rsg.h"
#include "scene.h"
#include "renderer.h"
#include <rhi/shader_manager.h>

FXAAPass::FXAAPass(Renderer* renderer)
{
    _renderer = renderer;

    EzSamplerDesc sampler_desc{};
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);
}

FXAAPass::~FXAAPass()
{
    if (_sampler)
        ez_destroy_sampler(_sampler);
}

void FXAAPass::render()
{
    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[2];
    rt_barriers[0] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_SHADER_RESOURCE);
    rt_barriers[1] = ez_image_barrier(_renderer->_post_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    ez_pipeline_barrier(0, 0, nullptr, 2, rt_barriers);

    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_post_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
    EzRenderingInfo rendering_info{};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.colors.push_back(color_info);
    ez_begin_rendering(rendering_info);

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://fxaa.vert"));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://fxaa.frag"));

    ez_set_vertex_binding(0, 20);
    ez_set_vertex_attrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    ez_set_vertex_attrib(0, 1, VK_FORMAT_R32G32_SFLOAT, 12);

    ez_bind_texture(0, _renderer->_color_rt, 0);
    ez_bind_sampler(1, _sampler);

    ez_set_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    ez_bind_vertex_buffer(RSG::quad_buffer);
    ez_draw(6, 0);

    ez_end_rendering();
}