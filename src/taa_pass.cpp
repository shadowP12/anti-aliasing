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
}

void TAAPass::set_dirty()
{
    _dirty = true;
}

void TAAPass::render()
{
    if (_dirty)
    {
        _dirty = false;
    }
}