#ifndef VKPLAYGROUND_APPLICATION_HPP
#define VKPLAYGROUND_APPLICATION_HPP

#include <iostream>
#include <vector>

#if defined(__unix__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(_MSVC_VER)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <shader.hpp>
#include <callbacks.hpp>

namespace vk_playground {
    class application {
        constexpr static const char* enabled_layers[] = { "VK_LAYER_KHRONOS_validation" };
        constexpr static const char* enabled_device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        constexpr static const int max_frames_in_flight = 2;

        constexpr static bool enable_validation_layers =
#if defined(_GLIBCXX_DEBUG) || defined(DEBUG)
            true;
#else
            false;
#endif

        std::vector<const char*> enabled_extensions{};

        constexpr static const int width = 1280;
        constexpr static const int height = 720;

        std::vector<VkExtensionProperties> extensions{};
        std::vector<VkQueueFamilyProperties> queue_families{};
        std::vector<VkImage> swapchain_images{};
        std::vector<VkImageView> swapchain_image_views{};
        std::vector<VkFramebuffer> swapchain_framebuffers{};

        VkInstance instance{};
        VkDebugUtilsMessengerEXT debug_messenger{};
        VkPhysicalDevice physical_device{};
        VkDevice device{};
        VkQueue queue_handle{};
        VkCommandPool command_pool{};
        std::vector<VkCommandBuffer> command_buffers{};
        VkSurfaceKHR surface{};
        VkSwapchainKHR swapchain{};
        struct final_swapchain {
            VkSurfaceFormatKHR format;
            VkPresentModeKHR present_mode;
            VkExtent2D resolution;
            std::uint32_t image_count;
        } swapchain_info{};
        std::vector<shader> shader_modules{};
        VkRenderPass render_pass{};
        VkPipelineLayout pipeline_layout{};
        VkPipeline graphics_pipeline{};

        std::vector<VkSemaphore> image_available{};
        std::vector<VkSemaphore> render_finish{};

        GLFWwindow* window{};

        void setup_debug_callback();
        void enable_required_extensions();
        void create_instance();
        void create_surface();
        void enable_all_extensions();
        void init_physical_device();
        void init_queues_families();
        size_t get_graphics_queue_index() const;
        void create_device();
        void init_command_pool();
        void init_command_buffer();
        void create_swapchain();
        void create_image_views();
        void create_shader_modules();
        void create_render_pass();
        void create_pipeline();
        void create_framebuffer();
        void create_semaphores();

        void draw_frame();

    public:
        application() = default;
        ~application();

        void glfw_init();
        void vk_init();
        void run();
    };
} // namespace vk_playground

#endif //VKPLAYGROUND_APPLICATION_HPP
