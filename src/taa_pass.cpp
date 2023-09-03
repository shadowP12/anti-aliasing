#include "taa_pass.h"
#include "rsg.h"
#include "scene.h"
#include "renderer.h"
#include <rhi/shader_manager.h>

struct TAAConstant
{
    glm::vec2 resolution;
};

TAAPass::TAAPass(Renderer* renderer)
{
    _renderer = renderer;

    EzSamplerDesc sampler_desc{};
    sampler_desc.address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_desc.address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    ez_create_sampler(sampler_desc, _sampler);
}

TAAPass::~TAAPass()
{
    if (_taa_history)
        ez_destroy_texture(_taa_history);

    if (_taa_temp)
        ez_destroy_texture(_taa_temp);

    if (_taa_prev_velocity)
        ez_destroy_texture(_taa_prev_velocity);

    if (_sampler)
        ez_destroy_sampler(_sampler);
}

void TAAPass::set_dirty()
{
    _dirty = true;
}

void TAAPass::render()
{
    VkImageMemoryBarrier2 barriers[6];
    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent = { _renderer->_width, _renderer->_height, 1 };

    if (_dirty)
    {
        if (_taa_history)
            ez_destroy_texture(_taa_history);

        if (_taa_temp)
            ez_destroy_texture(_taa_temp);

        if (_taa_prev_velocity)
            ez_destroy_texture(_taa_prev_velocity);

        EzTextureDesc desc{};
        desc.width = _renderer->_width;
        desc.height = _renderer->_height;
        desc.format = VK_FORMAT_B8G8R8A8_UNORM;
        desc.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ez_create_texture(desc, _taa_history);
        ez_create_texture_view(_taa_history, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
        ez_create_texture(desc, _taa_temp);
        ez_create_texture_view(_taa_temp, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

        desc.format = VK_FORMAT_R16G16_SFLOAT;
        ez_create_texture(desc, _taa_prev_velocity);
        ez_create_texture_view(_taa_prev_velocity, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    }
    else
    {
        // Resolve
        barriers[0] = ez_image_barrier(_taa_temp, EZ_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[2] = ez_image_barrier(_renderer->_depth_rt, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        barriers[3] = ez_image_barrier(_renderer->_velocity_rt, EZ_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[4] = ez_image_barrier(_taa_prev_velocity, EZ_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[5] = ez_image_barrier(_taa_history, EZ_RESOURCE_STATE_SHADER_RESOURCE);
        ez_pipeline_barrier(0, 0, nullptr, 6, barriers);

        ez_reset_pipeline_state();
        ez_set_compute_shader(ShaderManager::get()->get_shader("shader://taa_resolve.comp"));

        ez_bind_texture(0, _taa_temp, 0);
        ez_bind_texture(1, _renderer->_color_rt, 0);
        ez_bind_texture(2, _renderer->_depth_rt, 0);
        ez_bind_texture(3, _renderer->_velocity_rt, 0);
        ez_bind_texture(4, _taa_prev_velocity, 0);
        ez_bind_texture(5, _taa_history, 0);
        ez_bind_sampler(6, _sampler);

        TAAConstant taa_constant;
        taa_constant.resolution.x = _renderer->_width;
        taa_constant.resolution.y = _renderer->_height;
        ez_push_constants(&taa_constant, sizeof(TAAConstant), 0);

        ez_dispatch(std::max(1u, (uint32_t)(_renderer->_width) / 16), std::max(1u, (uint32_t)(_renderer->_height) / 16), 1);

        barriers[0] = ez_image_barrier(_taa_temp, EZ_RESOURCE_STATE_COPY_SOURCE);
        barriers[1] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_COPY_DEST);
        ez_pipeline_barrier(0, 0, nullptr, 2, barriers);
        ez_copy_image(_taa_temp, _renderer->_color_rt, copy_region);
    }

    barriers[0] = ez_image_barrier(_renderer->_color_rt, EZ_RESOURCE_STATE_COPY_SOURCE);
    barriers[1] = ez_image_barrier(_renderer->_velocity_rt, EZ_RESOURCE_STATE_COPY_SOURCE);
    barriers[2] = ez_image_barrier(_taa_history, EZ_RESOURCE_STATE_COPY_DEST);
    barriers[3] = ez_image_barrier(_taa_prev_velocity, EZ_RESOURCE_STATE_COPY_DEST);
    ez_pipeline_barrier(0, 0, nullptr, 4, barriers);
    ez_copy_image(_renderer->_color_rt, _taa_history, copy_region);
    ez_copy_image(_renderer->_velocity_rt, _taa_prev_velocity, copy_region);

    _dirty = false;
}