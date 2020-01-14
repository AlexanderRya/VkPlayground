#ifndef VKPLAYGROUND_SHADER_HPP
#define VKPLAYGROUND_SHADER_HPP

#include <filesystem>
#include <fstream>
#include <vulkan/vulkan.h>

namespace vk_playground {
    class shader {
        std::array<VkShaderModule, 2> shader_module{};
        std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};

        std::string vertex_spv{}, fragment_spv{};

    public:
        shader(const std::filesystem::path&, const std::filesystem::path&);

        void create_module(const VkDevice&);
        std::array<VkShaderModule, 2>& get_modules();
    };
} // namespace vk_playground

#endif //VKPLAYGROUND_SHADER_HPP
