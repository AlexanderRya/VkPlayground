#include "application.hpp"

namespace vk_playground {
    void application::vk_init() {
        enable_required_extensions();
        create_instance();
        setup_debug_callback();
        create_surface();
        init_physical_device();
        init_queues_families();
        create_device();
        init_command_pool();
        init_command_buffer();
        create_swapchain();
    }

    void application::glfw_init() {
        if (!glfwInit()) {
            throw std::runtime_error("Failed glfw init\n");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan Playground", nullptr, nullptr);
    }

    application::~application() {
        vkDestroyCommandPool(device, command_pool, nullptr);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyDevice(device, nullptr);
        if (enable_validation_layers) {
            auto destroy_debug = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            destroy_debug(instance, debug_messenger, nullptr);
        }
        vkDestroyInstance(instance, nullptr);


        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void application::run() const {
        while (!glfwWindowShouldClose(window)) {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
        }
    }

    void application::create_instance() {
        VkInstanceCreateInfo info{}; {
            info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            if (enable_validation_layers) {
                info.enabledLayerCount = 1;
                info.ppEnabledLayerNames = enabled_layers;
            } else {
                info.enabledLayerCount = 0;
            }
            info.enabledExtensionCount = enabled_extensions.size();
            info.ppEnabledExtensionNames = enabled_extensions.data();
        }

        if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Could not create instance\n");
        }
    }

    void application::create_surface() {
        glfwCreateWindowSurface(instance, window, nullptr, &surface);
    }

    void application::enable_all_extensions() {
        std::uint32_t extension_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

        extensions.resize(extension_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

        for (const auto& extension : extensions) {
            enabled_extensions.emplace_back(extension.extensionName);
        }
    }

    void application::init_physical_device() {
        std::uint32_t total_devices;
        vkEnumeratePhysicalDevices(instance, &total_devices, nullptr);

        std::vector<VkPhysicalDevice> physical_devices(total_devices);
        vkEnumeratePhysicalDevices(instance, &total_devices, physical_devices.data());

        for (const auto& dev : physical_devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);

            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                physical_device = dev;
                break;
            }
        }

        if (physical_device == nullptr) {
            throw std::runtime_error("Error, can't find a discrete gpu with vulkan support");
        }
    }

    void application::init_queues_families() {
        std::uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

        queue_families.resize(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
    }

    int application::get_graphics_queue_index() const {
        for (int i = 0; i < queue_families.size(); ++i) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && present_support) {
                return i;
            }
        }

        return -1;
    }

    void application::create_device() {
        int graphics_queue_index = get_graphics_queue_index();

        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info{}; {
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = graphics_queue_index;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
        }

        VkDeviceCreateInfo device_create_info{}; {
            device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            device_create_info.pQueueCreateInfos = &queue_create_info;
            device_create_info.queueCreateInfoCount = 1;
            device_create_info.ppEnabledExtensionNames = enabled_device_extensions;
            device_create_info.enabledExtensionCount = 1;
        }

        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed creating logical device\n");
        }

        vkGetDeviceQueue(device, graphics_queue_index, 0, &queue_handle);
    }

    void application::init_command_pool() {
        VkCommandPoolCreateInfo command_pool_info{}; {
            command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            command_pool_info.queueFamilyIndex = get_graphics_queue_index();
            command_pool_info.flags = 0;
        }

        if (vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed creating command pool\n");
        }
    }

    void application::init_command_buffer() {
        VkCommandBufferAllocateInfo command_buf_info{}; {
            command_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buf_info.commandPool = command_pool;
            command_buf_info.commandBufferCount = 1;
            command_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }

        if (vkAllocateCommandBuffers(device, &command_buf_info, &command_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed creating command pool\n");
        }
    }

    void application::create_swapchain() {
        VkSurfaceCapabilitiesKHR surface_capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

        if (surface_capabilities.currentExtent.width != UINT32_MAX) {
            swapchain_info.resolution = surface_capabilities.currentExtent;
        } else {
            VkExtent2D actual_extent{ width, height };

            actual_extent.width = std::clamp(actual_extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            actual_extent.height = std::clamp(actual_extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

            swapchain_info.resolution = actual_extent;
        }
        swapchain_info.image_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount > 0 && swapchain_info.image_count > surface_capabilities.maxImageCount) {
            swapchain_info.image_count = surface_capabilities.maxImageCount;
        }

        std::uint32_t format_count{};
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());

        swapchain_info.format = formats[0];
        for (const auto& format : formats) {
            if ((format.format == VK_FORMAT_B8G8R8A8_SRGB ||
                format.format == VK_FORMAT_R8G8B8A8_SRGB) &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                swapchain_info.format = format;
                break;
            }
        }

        std::uint32_t present_mode_count{};
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
        std::vector<VkPresentModeKHR> present_modes(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes.data());

        swapchain_info.present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& mode : present_modes) {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                swapchain_info.present_mode = mode;
            }
        }

        VkSwapchainCreateInfoKHR swapchain_create_info = {}; {
            swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchain_create_info.surface = surface;
            swapchain_create_info.minImageCount = swapchain_info.image_count;
            swapchain_create_info.imageFormat = swapchain_info.format.format;
            swapchain_create_info.imageColorSpace = swapchain_info.format.colorSpace;
            swapchain_create_info.imageExtent = swapchain_info.resolution;
            swapchain_create_info.imageArrayLayers = 1;
            swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchain_create_info.queueFamilyIndexCount = 0;
            swapchain_create_info.pQueueFamilyIndices = nullptr;
            swapchain_create_info.preTransform = surface_capabilities.currentTransform;
            swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchain_create_info.presentMode = swapchain_info.present_mode;
            swapchain_create_info.clipped = true;
            swapchain_create_info.oldSwapchain = nullptr;
        }

        if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_info.image_count, nullptr);
        swapchain_images.resize(swapchain_info.image_count);
        vkGetSwapchainImagesKHR(device, swapchain, &swapchain_info.image_count, swapchain_images.data());
    }

    void application::setup_debug_callback() {
        if (!enable_validation_layers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {}; {
            debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_create_info.pfnUserCallback = vulkan_debug_callback;
            debug_create_info.pUserData = nullptr;
        }

        auto create_debug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        create_debug(instance, &debug_create_info, nullptr, &debug_messenger);
    }

    void application::enable_required_extensions() {
        std::uint32_t req_count{};
        auto req_extensions = glfwGetRequiredInstanceExtensions(&req_count);
        for (int i = 0; i < req_count; ++i) {
            enabled_extensions.emplace_back(req_extensions[i]);
        }

        enabled_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
} // namespace vk_playground