#include "shader.hpp"

namespace vk_playground {
    shader::shader(const std::filesystem::path& vert, const std::filesystem::path& frag) {
        std::ifstream fvert(vert.generic_string(), std::ios::binary);
        std::ifstream ffrag(frag.generic_string(), std::ios::binary);

        if (!fvert.is_open()) {
            throw std::runtime_error("Error, " + vert.generic_string() + " not found");
        }

        if (!ffrag.is_open()) {
            throw std::runtime_error("Error, " + frag.generic_string() + " not found");
        }

        vertex_spv = std::string{ std::istreambuf_iterator{ fvert }, {} };
        fragment_spv = std::string{ std::istreambuf_iterator{ ffrag }, {} };
    }

    void shader::create_module(const VkDevice& device) {
        VkShaderModuleCreateInfo vertex_module_info{}; {
            vertex_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            vertex_module_info.codeSize = vertex_spv.size();
            vertex_module_info.pCode = reinterpret_cast<const uint32_t*>(vertex_spv.data());
        }

        if (vkCreateShaderModule(device, &vertex_module_info, nullptr, &shader_module[0]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create vertex shader module!");
        }

        VkShaderModuleCreateInfo fragment_module_info{}; {
            fragment_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            fragment_module_info.codeSize = fragment_spv.size();
            fragment_module_info.pCode = reinterpret_cast<const uint32_t*>(fragment_spv.data());
        }

        if (vkCreateShaderModule(device, &fragment_module_info, nullptr, &shader_module[1]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fragment shader module!");
        }
    }

    std::array<VkShaderModule, 2>& shader::get_modules() {
        return shader_module;
    }
} // namespace vk_playground
