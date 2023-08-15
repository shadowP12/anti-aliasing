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
    bool _dirty = true;
};