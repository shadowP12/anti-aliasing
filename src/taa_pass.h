#pragma once
#include <rhi/ez_vulkan.h>

class Renderer;

class TAAPass
{
public:
    TAAPass(Renderer* renderer);

    ~TAAPass();

    void set_dirty();

    void render();

private:
    Renderer* _renderer;
    EzTexture _taa_history = VK_NULL_HANDLE;
    EzTexture _taa_temp = VK_NULL_HANDLE;
    EzTexture _taa_prev_velocity = VK_NULL_HANDLE;
    EzSampler _sampler = VK_NULL_HANDLE;
    bool _dirty = true;
};