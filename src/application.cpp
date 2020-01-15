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
        create_swapchain();
        create_image_views();
        init_command_pool();
        init_command_buffer();
        create_shader_modules();
        create_render_pass();
        create_pipeline();
        create_framebuffer();
        create_semaphores();
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
        for (const auto& framebuffer : swapchain_framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        vkDestroyPipeline(device, graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);
        for (auto& module : shader_modules) {
            const auto& [vert, frag] = module.get_modules();
            vkDestroyShaderModule(device, vert, nullptr);
            vkDestroyShaderModule(device, frag, nullptr);
        }
        for (const auto& image_view : swapchain_image_views) {
            vkDestroyImageView(device, image_view, nullptr);
        }
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

    void application::run() {
        while (!glfwWindowShouldClose(window)) {
            draw_frame();
        }
        vkDeviceWaitIdle(device);
    }

    void application::create_instance() {
        VkApplicationInfo application_info = {}; {
            application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            application_info.pApplicationName = "VkPlayground";
            application_info.applicationVersion = VK_API_VERSION_1_1;
            application_info.pEngineName = "no u";
            application_info.engineVersion = VK_API_VERSION_1_1;
            application_info.apiVersion = VK_API_VERSION_1_1;
        }

        VkInstanceCreateInfo info{}; {
            info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            info.pApplicationInfo = &application_info;
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
            throw std::runtime_error("Could not create instance");
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

    size_t application::get_graphics_queue_index() const {
        for (size_t i = 0; i < queue_families.size(); ++i) {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
            if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                present_support) {
                return i;
            }
        }

        return -1;
    }

    void application::create_device() {
        auto graphics_queue_index = get_graphics_queue_index();

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
            throw std::runtime_error("Failed creating logical device");
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
            throw std::runtime_error("Failed creating command pool");
        }
    }

    void application::init_command_buffer() {
        command_buffers.resize(swapchain_info.image_count);
        VkCommandBufferAllocateInfo command_buf_info{}; {
            command_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            command_buf_info.commandPool = command_pool;
            command_buf_info.commandBufferCount = swapchain_info.image_count;
            command_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        }

        if (vkAllocateCommandBuffers(device, &command_buf_info, command_buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed creating command pool");
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

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{}; {
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

    void application::create_image_views() {
        VkImageViewCreateInfo image_view_info{}; {
            image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            image_view_info.format = swapchain_info.format.format;
            image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_view_info.subresourceRange.baseMipLevel = 0;
            image_view_info.subresourceRange.levelCount = 1;
            image_view_info.subresourceRange.baseArrayLayer = 0;
            image_view_info.subresourceRange.layerCount = 1;
        }

        for (const auto& image : swapchain_images) {
            image_view_info.image = image;
            if (vkCreateImageView(device, &image_view_info, nullptr, &swapchain_image_views.emplace_back()) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view");
            }
        }
    }

    void application::create_shader_modules() {
        shader_modules.emplace_back(
            "../resources/shaders/compiled/triangle_vert.spv",
            "../resources/shaders/compiled/triangle_frag.spv").create_module(device);
    }

    void application::create_render_pass() {
        VkAttachmentDescription color_attachment{}; {
            color_attachment.format = swapchain_info.format.format;
            color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }

        VkAttachmentReference color_attachment_ref{}; {
            color_attachment_ref.attachment = 0;
            color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass_description{}; {
            subpass_description.colorAttachmentCount = 1;
            subpass_description.pColorAttachments = &color_attachment_ref;
            subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        }

        VkRenderPassCreateInfo render_pass_info{}; {
            render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            render_pass_info.attachmentCount = 1;
            render_pass_info.pAttachments = &color_attachment;
            render_pass_info.subpassCount = 1;
            render_pass_info.pSubpasses = &subpass_description;
        }

        if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create renderpass");
        }
    }

    void application::create_pipeline() {
        VkPipelineShaderStageCreateInfo vert_pipeline_create_info{}; {
            vert_pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vert_pipeline_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vert_pipeline_create_info.module = shader_modules.back().get_modules()[0];
            vert_pipeline_create_info.pName = "main";
        }

        VkPipelineShaderStageCreateInfo frag_pipeline_create_info{}; {
            frag_pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            frag_pipeline_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            frag_pipeline_create_info.module = shader_modules.back().get_modules()[1];
            frag_pipeline_create_info.pName = "main";
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_info{}; {
            vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertex_input_info.vertexBindingDescriptionCount = 0;
            vertex_input_info.pVertexBindingDescriptions = nullptr;
            vertex_input_info.vertexAttributeDescriptionCount = 0;
            vertex_input_info.pVertexAttributeDescriptions = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo input_assembly{}; {
            input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly.primitiveRestartEnable = false;
        }

        VkViewport viewport{}; {
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapchain_info.resolution.width);
            viewport.height = static_cast<float>(swapchain_info.resolution.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
        }

        VkRect2D scissor{}; {
            scissor.extent = swapchain_info.resolution;
            scissor.offset = { 0, 0 };
        }

        VkPipelineViewportStateCreateInfo viewport_state_info{}; {
            viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewport_state_info.viewportCount = 1;
            viewport_state_info.pViewports = &viewport;
            viewport_state_info.scissorCount = 1;
            viewport_state_info.pScissors = &scissor;
        }

        VkPipelineRasterizationStateCreateInfo rasterizer_state_info{}; {
            rasterizer_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer_state_info.depthClampEnable = false;
            rasterizer_state_info.rasterizerDiscardEnable = false;
            rasterizer_state_info.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer_state_info.lineWidth = 1.0f;
            rasterizer_state_info.cullMode = VK_CULL_MODE_NONE;
            rasterizer_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer_state_info.depthBiasEnable = false;
        }

        VkPipelineMultisampleStateCreateInfo multisampling_state_info{}; {
            multisampling_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling_state_info.sampleShadingEnable = false;
            multisampling_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling_state_info.minSampleShading = 1.0f;
            multisampling_state_info.pSampleMask = nullptr;
            multisampling_state_info.alphaToCoverageEnable = false;
            multisampling_state_info.alphaToOneEnable = false;
        }

        VkPipelineColorBlendAttachmentState color_blend_attachment{}; {
            color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                    VK_COLOR_COMPONENT_G_BIT |
                                                    VK_COLOR_COMPONENT_B_BIT |
                                                    VK_COLOR_COMPONENT_A_BIT;
            color_blend_attachment.blendEnable = false;
            color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }

        VkPipelineColorBlendStateCreateInfo color_blend_info{}; {
            color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            color_blend_info.logicOpEnable = false;
            color_blend_info.logicOp = VK_LOGIC_OP_COPY;
            color_blend_info.attachmentCount = 1;
            color_blend_info.pAttachments = &color_blend_attachment;
            color_blend_info.blendConstants[0] = 0.0f;
            color_blend_info.blendConstants[1] = 0.0f;
            color_blend_info.blendConstants[2] = 0.0f;
            color_blend_info.blendConstants[3] = 0.0f;
        }

        VkPipelineLayoutCreateInfo pipeline_layout_info{}; {
            pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipeline_layout_info.setLayoutCount = 0;
            pipeline_layout_info.pSetLayouts = nullptr;
            pipeline_layout_info.pushConstantRangeCount = 0;
            pipeline_layout_info.pPushConstantRanges = nullptr;
        }

        if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkPipelineShaderStageCreateInfo shader_stages[] = { vert_pipeline_create_info, frag_pipeline_create_info };

        VkGraphicsPipelineCreateInfo pipeline_info{}; {
            pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipeline_info.stageCount = 2;
            pipeline_info.pStages = shader_stages;
            pipeline_info.pVertexInputState = &vertex_input_info;
            pipeline_info.pInputAssemblyState = &input_assembly;
            pipeline_info.pViewportState = &viewport_state_info;
            pipeline_info.pRasterizationState = &rasterizer_state_info;
            pipeline_info.pMultisampleState = &multisampling_state_info;
            pipeline_info.pDepthStencilState = nullptr;
            pipeline_info.pColorBlendState = &color_blend_info;
            pipeline_info.pDynamicState = nullptr;
            pipeline_info.layout = pipeline_layout;
            pipeline_info.renderPass = render_pass;
            pipeline_info.subpass = 0;
            pipeline_info.basePipelineHandle = nullptr;
            pipeline_info.basePipelineIndex = -1;
        }

       if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
       }
    }

    void application::create_framebuffer() {
        swapchain_framebuffers.resize(swapchain_image_views.size());

        for (size_t i = 0; i < swapchain_image_views.size(); i++) {
            VkFramebufferCreateInfo framebuffer_info{}; {
                framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebuffer_info.renderPass = render_pass;
                framebuffer_info.attachmentCount = 1;
                framebuffer_info.pAttachments = &swapchain_image_views[i];
                framebuffer_info.width = swapchain_info.resolution.width;
                framebuffer_info.height = swapchain_info.resolution.height;
                framebuffer_info.layers = 1;
            }

            if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swapchain_framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }

        VkCommandBufferBeginInfo cmd_buf_begin_info{}; {
            cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_begin_info.flags = 0;
            cmd_buf_begin_info.pInheritanceInfo = nullptr;
        }

        for (size_t i = 0; i < swapchain_framebuffers.size(); ++i) {
            vkBeginCommandBuffer(command_buffers[i], &cmd_buf_begin_info);

            VkRenderPassBeginInfo render_pass_begin_info{}; {
                render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                render_pass_begin_info.renderPass = render_pass;
                render_pass_begin_info.framebuffer = swapchain_framebuffers[i];
                render_pass_begin_info.renderArea.offset = { 0, 0 };
                render_pass_begin_info.renderArea.extent = swapchain_info.resolution;
                VkClearValue clear_color{ 0.0f, 0.0f, 0.0f, 1.0f };
                render_pass_begin_info.clearValueCount = 1;
                render_pass_begin_info.pClearValues = &clear_color;

                vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
                vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
                vkCmdEndRenderPass(command_buffers[i]);
            }

            vkEndCommandBuffer(command_buffers[i]);
        }
    }

    void application::draw_frame() {
        static std::size_t current_frame = 0;

        vkWaitForFences(device, 1, &frames_in_flight[current_frame], true, UINT64_MAX);

        std::uint32_t image_index{};
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available[current_frame], nullptr, &image_index);

        if (images_in_flight[image_index] != nullptr) {
            vkWaitForFences(device, 1, &images_in_flight[image_index], true, UINT64_MAX);
        }

        images_in_flight[image_index] = frames_in_flight[current_frame];

        constexpr VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info{}; {
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffers[image_index];
            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &render_finish[current_frame];
            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &image_available[current_frame];
            submit_info.pWaitDstStageMask = &pipeline_stage_flags;
        }

        vkResetFences(device, 1, &frames_in_flight[current_frame]);

        if (vkQueueSubmit(queue_handle, 1, &submit_info, frames_in_flight[current_frame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit command buffer");
        }

        VkPresentInfoKHR present_info{}; {
            present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &render_finish[current_frame];
            present_info.pImageIndices = &image_index;
            present_info.pSwapchains = &swapchain;
            present_info.swapchainCount = 1;
        }

        vkQueuePresentKHR(queue_handle, &present_info);
        current_frame = (current_frame + 1) % max_frames_in_flight;
    }

    void application::create_semaphores() {
        image_available.resize(max_frames_in_flight, {});
        render_finish.resize(max_frames_in_flight, {});
        frames_in_flight.resize(max_frames_in_flight, {});
        images_in_flight.resize(swapchain_info.image_count, {});

        VkSemaphoreCreateInfo semaphore_create_info{}; {
            semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        }

        VkFenceCreateInfo fence_create_info{}; {
            fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        for (int i = 0; i < max_frames_in_flight; ++i) {
            if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finish[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_create_info, nullptr, &frames_in_flight[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_create_info, nullptr, &images_in_flight[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create semaphores and fences");
            }
        }
    }
} // namespace vk_playground