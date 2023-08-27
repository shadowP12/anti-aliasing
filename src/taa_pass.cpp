#include "taa_pass.h"
#include "rsg.h"
#include "scene.h"
#include "renderer.h"
#include <rhi/shader_manager.h>

TAAPass::TAAPass(Renderer* renderer)
{
    _renderer = renderer;
}

TAAPass::~TAAPass()
{
    if (_taa_history)
        ez_destroy_texture(_taa_history);

    if (_taa_temp)
        ez_destroy_texture(_taa_temp);

    if (_taa_prev_velocity)
        ez_destroy_texture(_taa_prev_velocity);
}

void TAAPass::set_dirty()
{
    _dirty = true;
}

void TAAPass::render()
{
    VkImageMemoryBarrier2 barriers[3];

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
        desc.format = VK_FORMAT_R8G8B8A8_UNORM;
        desc.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        ez_create_texture(desc, _taa_history);
        ez_create_texture_view(_taa_history, VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1);
        ez_create_texture(desc, _taa_temp);
        ez_create_texture_view(_taa_temp, VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1);
        desc.format = VK_FORMAT_R16G16_SFLOAT;
        ez_create_texture(desc, _taa_prev_velocity);
        ez_create_texture_view(_taa_prev_velocity, VK_IMAGE_VIEW_TYPE_2D, 0, 1, 0, 1);
    }

    _dirty = false;
}