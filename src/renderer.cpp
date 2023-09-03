#include "renderer.h"
#include "camera.h"
#include "scene.h"
#include "rsg.h"
#include "base_pass.h"
#include "fxaa_pass.h"
#include "taa_pass.h"

const int TAA_JITTER_COUNT = 16;
std::vector<glm::vec2> taa_jitter_array;

float get_halton_value(int index, int base)
{
    float f = 1;
    float r = 0;
    while (index > 0)
    {
        f = f / static_cast<float>(base);
        r = r + f * (index % base);
        index = index / base;
    }
    return r * 2.0f - 1.0f;
};

Renderer::Renderer()
{
    init_rsg();

    _base_pass = new BasePass(this);
    _fxaa = new FXAAPass(this);
    _taa = new TAAPass(this);

    EzBufferDesc buffer_desc{};
    buffer_desc.size = sizeof(ViewBufferType);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _view_buffer);

    taa_jitter_array.resize(TAA_JITTER_COUNT);
    for (int i = 0; i < TAA_JITTER_COUNT; i++)
    {
        taa_jitter_array[i].x = get_halton_value(i, 2);
        taa_jitter_array[i].y = get_halton_value(i, 3);
    }
}

Renderer::~Renderer()
{
    uninit_rsg();

    delete _base_pass;
    delete _fxaa;
    delete _taa;

    if (_scene_buffer)
        ez_destroy_buffer(_scene_buffer);
    if (_view_buffer)
        ez_destroy_buffer(_view_buffer);
    if (_resolve_rt)
        ez_destroy_texture(_resolve_rt);
    if (_color_rt)
        ez_destroy_texture(_color_rt);
    if(_depth_rt)
        ez_destroy_texture(_depth_rt);
    if(_velocity_rt)
        ez_destroy_texture(_velocity_rt);
    if(_post_rt)
        ez_destroy_texture(_post_rt);
}

void Renderer::set_scene(Scene* scene)
{
    if (_scene != scene)
    {
        _scene = scene;
        _scene_dirty = true;
    }
}

void Renderer::set_camera(Camera* camera)
{
    _camera = camera;
}

void Renderer::set_aa(Renderer::AA aa)
{
    _aa = aa;
}

void Renderer::update_rendertarget()
{
    EzTextureDesc desc{};
    desc.width = _width;
    desc.height = _height;
    desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    if (_aa == AA::MSAA)
    {
        if (_resolve_rt)
            ez_destroy_texture(_resolve_rt);
        ez_create_texture(desc, _resolve_rt);
        ez_create_texture_view(_resolve_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
        desc.samples = VK_SAMPLE_COUNT_4_BIT;
    }
    else if (_aa == AA::FXAA)
    {
        if (_post_rt)
            ez_destroy_texture(_post_rt);
        ez_create_texture(desc, _post_rt);
        ez_create_texture_view(_post_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    }
    if (_color_rt)
        ez_destroy_texture(_color_rt);
    ez_create_texture(desc, _color_rt);
    ez_create_texture_view(_color_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);

    desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
    desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (_depth_rt)
        ez_destroy_texture(_depth_rt);
    ez_create_texture(desc, _depth_rt);
    ez_create_texture_view(_depth_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1);

    if (_aa == AA::TAA)
    {
        desc.format = VK_FORMAT_R16G16_SFLOAT;
        desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (_velocity_rt)
            ez_destroy_texture(_velocity_rt);
        ez_create_texture(desc, _velocity_rt);
        ez_create_texture_view(_velocity_rt, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
    }

    _taa->set_dirty();
}

void Renderer::update_scene_buffer()
{
    std::vector<SceneBufferType> elements;
    for (auto node : _scene->nodes)
    {
        SceneBufferType element{};
        element.transform = node->transform;
        elements.push_back(element);
    }
    if (_scene_buffer)
        ez_destroy_buffer(_scene_buffer);

    EzBufferDesc buffer_desc{};
    buffer_desc.size = elements.size() * sizeof(SceneBufferType);
    buffer_desc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_desc.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ez_create_buffer(buffer_desc, _scene_buffer);

    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(_scene_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(_scene_buffer, elements.size() * sizeof(SceneBufferType), 0, elements.data());

    VkPipelineStageFlags2 stage_flags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkAccessFlags2 access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier = ez_buffer_barrier(_scene_buffer, stage_flags, access_flags);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);
}

void Renderer::update_view_buffer()
{
    ViewData view_data{};
    view_data.view_matrix = _camera->get_view_matrix();
    view_data.proj_matrix = _camera->get_proj_matrix();
    view_data.view_position = glm::vec4(_camera->get_translation(), 0.0f);

    if (_aa == TAA)
    {
        view_data.taa_jitter = taa_jitter_array[_frame_number % TAA_JITTER_COUNT] / glm::vec2(_width, _height) * 0.3f;
        view_data.proj_matrix[3][0] += view_data.taa_jitter.x;
        view_data.proj_matrix[3][1] += view_data.taa_jitter.y;
    }

    if (_frame_number == 0)
        _last_view_data = view_data;

    ViewBufferType view_buffer_type{};
    view_buffer_type.cur = view_data;
    view_buffer_type.prev = _last_view_data;

    VkBufferMemoryBarrier2 barrier = ez_buffer_barrier(_view_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    ez_update_buffer(_view_buffer, sizeof(ViewBufferType), 0, &view_buffer_type);

    VkPipelineStageFlags2 stage_flags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    VkAccessFlags2 access_flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier = ez_buffer_barrier(_view_buffer, stage_flags, access_flags);
    ez_pipeline_barrier(0, 1, &barrier, 0, nullptr);

    _last_view_data = view_data;
}

void Renderer::render(EzSwapchain swapchain)
{
    if (!_scene || !_camera)
        return;

    if (swapchain->width == 0 || swapchain->height ==0)
        return;

    if (_width != swapchain->width || _height != swapchain->height)
    {
        _width = swapchain->width;
        _height = swapchain->height;
        update_rendertarget();
    }
    if (_scene_dirty)
    {
        update_scene_buffer();
        _scene_dirty = false;
    }
    update_view_buffer();

    // Render passes
    _base_pass->render();
    if (_aa == FXAA)
        _fxaa->render();
    else if (_aa == TAA)
        _taa->render();

    // Copy to swapchain
    EzTexture src_rt = _color_rt;
    if (_aa == MSAA)
        src_rt = _resolve_rt;
    else if (_aa == FXAA)
        src_rt = _post_rt;

    VkImageMemoryBarrier2 copy_barriers[] = {
        ez_image_barrier(src_rt, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
        ez_image_barrier(swapchain, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT),
    };
    ez_pipeline_barrier(0, 0, nullptr, 2, copy_barriers);

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent = { swapchain->width, swapchain->height, 1 };
    ez_copy_image(src_rt, swapchain, copy_region);

    _frame_number++;
}
