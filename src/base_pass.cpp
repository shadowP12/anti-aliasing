#include "base_pass.h"
#include "scene.h"
#include "renderer.h"
#include <rhi/shader_manager.h>

BasePass::BasePass(Renderer* renderer)
{
    _renderer = renderer;
}

void BasePass::render()
{
    ez_reset_pipeline_state();

    VkImageMemoryBarrier2 rt_barriers[3];
    rt_barriers[0] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_RENDERTARGET);
    rt_barriers[1] = ez_image_barrier(_renderer->_depth_rt, EZ_RESOURCE_STATE_DEPTH_WRITE);
    if (_renderer->_aa == Renderer::MSAA)
    {
        rt_barriers[2] = ez_image_barrier(_renderer->_resolve_rt, EZ_RESOURCE_STATE_RENDERTARGET);
        ez_pipeline_barrier(0, 0, nullptr, 3, rt_barriers);
    }
    else if (_renderer->_aa == Renderer::TAA)
    {
        rt_barriers[2] = ez_image_barrier(_renderer->_velocity_rt, EZ_RESOURCE_STATE_RENDERTARGET);
        ez_pipeline_barrier(0, 0, nullptr, 3, rt_barriers);
    }
    else
    {
        ez_pipeline_barrier(0, 0, nullptr, 2, rt_barriers);
    }

    EzRenderingInfo rendering_info{};
    EzRenderingAttachmentInfo color_info{};
    color_info.texture = _renderer->_color_rt;
    color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
    if (_renderer->_aa == Renderer::MSAA)
    {
        color_info.resolve_texture = _renderer->_resolve_rt;
        color_info.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
    }
    rendering_info.colors.push_back(color_info);

    if (_renderer->_aa == Renderer::TAA)
    {
        color_info.texture = _renderer->_velocity_rt;
        color_info.clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
        rendering_info.colors.push_back(color_info);
    }

    EzRenderingAttachmentInfo depth_info{};
    depth_info.texture = _renderer->_depth_rt;
    depth_info.clear_value.depthStencil = {1.0f, 1};
    rendering_info.width = _renderer->_width;
    rendering_info.height = _renderer->_height;
    rendering_info.depth.push_back(depth_info);

    ez_begin_rendering(rendering_info);

    if (_renderer->_aa == Renderer::MSAA)
    {
        EzMultisampleState ms{};
        ms.sample_shading = true;
        ms.samples = VK_SAMPLE_COUNT_4_BIT;
        ez_set_multisample_state(ms);
    }

    ez_set_viewport(0, 0, (float)_renderer->_width, (float)_renderer->_height);
    ez_set_scissor(0, 0, (int32_t)_renderer->_width, (int32_t)_renderer->_height);

    std::vector<std::string> macros;
    if (_renderer->_aa == Renderer::TAA)
        macros.emplace_back("MOTION_VECTORS");
    ez_set_vertex_shader(ShaderManager::get()->get_shader("shader://scene.vert", macros));
    ez_set_fragment_shader(ShaderManager::get()->get_shader("shader://scene.frag", macros));

    ez_set_vertex_binding(0, 32);
    ez_set_vertex_attrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
    ez_set_vertex_attrib(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 12);
    ez_set_vertex_attrib(0, 2, VK_FORMAT_R32G32_SFLOAT, 24);

    ez_bind_buffer(1, _renderer->_view_buffer, sizeof(ViewBufferType));

    for (uint32_t i = 0; i < _renderer->_scene->nodes.size(); i++)
    {
        auto node = _renderer->_scene->nodes[i];
        if (node->mesh)
        {
            ez_bind_buffer(0, _renderer->_scene_buffer, sizeof(SceneBufferType), i * sizeof(SceneBufferType));
            for (auto prim : node->mesh->primitives)
            {
                ez_set_primitive_topology(prim->topology);
                ez_bind_vertex_buffer(prim->vertex_buffer);
                ez_bind_index_buffer(prim->index_buffer, prim->index_type);
                ez_draw_indexed(prim->index_count, 0, 0);
            }
        }
    }

    ez_end_rendering();
}