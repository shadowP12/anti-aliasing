#include <core/path.h>
#include <input/input.h>
#include <rhi/ez_vulkan.h>
#include <rhi/shader_manager.h>
#include <rhi/shader_compiler.h>
#include <scene/scene_tree.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static void window_size_callback(GLFWwindow* window, int w, int h)
{
}

static void cursor_position_callback(GLFWwindow* window, double pos_x, double pos_y)
{
    Input::get()->set_mouse_position((float)pos_x, (float)pos_y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Input::get()->set_mouse_button(button, action);
}

static void mouse_scroll_callback(GLFWwindow* window, double offset_x, double offset_y)
{
    Input::get()->set_mouse_scroll((float)offset_y);
}

int main()
{
    // Path settings
    Path::register_protocol("content", std::string(PROJECT_DIR) + "/content");
    Path::register_protocol("shaders", std::string(PROJECT_DIR) + "/content/shaders");

    ez_init();
    ShaderManager::get()->setup();
    ShaderCompiler::get()->setup();
    SceneTree::get()->setup();
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfw_window = glfwCreateWindow(800, 600, "anti-aliasing", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(glfw_window, window_size_callback);
    glfwSetCursorPosCallback(glfw_window, cursor_position_callback);
    glfwSetMouseButtonCallback(glfw_window, mouse_button_callback);
    glfwSetScrollCallback(glfw_window, mouse_scroll_callback);
    EzSwapchain swapchain = VK_NULL_HANDLE;
    ez_create_swapchain(glfwGetWin32Window(glfw_window), swapchain);

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        EzSwapchainStatus swapchain_status = ez_update_swapchain(swapchain);

        if (swapchain_status == EzSwapchainStatus::NotReady)
            continue;

        if (swapchain_status == EzSwapchainStatus::Resized)
        {
        }

        ez_acquire_next_image(swapchain);

        VkImageMemoryBarrier2 present_barrier[] = { ez_image_barrier(swapchain, EZ_RESOURCE_STATE_PRESENT) };
        ez_pipeline_barrier(0, 0, nullptr, 1, present_barrier);

        ez_present(swapchain);

        ez_submit();

        // Reset input
        Input::get()->reset();
    }

    ez_destroy_swapchain(swapchain);
    glfwDestroyWindow(glfw_window);
    glfwTerminate();
    ShaderManager::get()->cleanup();
    ShaderCompiler::get()->cleanup();
    SceneTree::get()->cleanup();
    ez_flush();
    ez_terminate();
    return 0;
}