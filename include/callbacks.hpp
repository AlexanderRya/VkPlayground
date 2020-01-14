#ifndef VKPLAYGROUND_CALLBACKS_HPP
#define VKPLAYGROUND_CALLBACKS_HPP

#include <util.hpp>
#include <fmt/format.h>

namespace vk_playground {
    static std::string get_message_type(const VkDebugUtilsMessageTypeFlagsEXT& type) {
        switch (type) {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: {
                return "General";
            }

            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: {
                return "Validation";
            }

            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: {
                return "Performance";
            }

            default: {
                return "Unknown";
            }
        }
    }

    static std::string get_message_severity(const VkDebugUtilsMessageSeverityFlagBitsEXT& type) {
        switch (type) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
                return "Verbose";
            }

            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
                return "Info";
            }

            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
                return "Warning";
            }

            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
                return "Error";
            }

            default: {
                return "Unknown";
            }
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void*) {

        std::cout << fmt::format("[{}] [{}] [{}]: {}\n",
                                 get_current_timestamp(),
                                 get_message_severity(message_severity),
                                 get_message_type(message_type),
                                 callback_data->pMessage);
        return 0;
    }
}

#endif //VKPLAYGROUND_CALLBACKS_HPP
