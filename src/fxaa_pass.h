#pragma once
#include <rhi/ez_vulkan.h>

class Renderer;

class FXAAPass
{
public:
    FXAAPass(Renderer* renderer);

    ~FXAAPass();

    void render();

private:
    Renderer* _renderer;
    EzSampler _sampler = VK_NULL_HANDLE;
};