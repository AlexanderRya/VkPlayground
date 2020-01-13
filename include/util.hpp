#ifndef VKPLAYGROUND_UTIL_HPP
#define VKPLAYGROUND_UTIL_HPP

#include <string>
#include <chrono>
#include <ctime>

namespace vk_playground {
    namespace {
        constexpr auto real_enum_field_name(std::string_view str) {
            return str.substr(str.find_last_of(':') + 1, str.size());
        }
    }

    template <auto Vx>
    constexpr auto enum_field_name() {
        return real_enum_field_name({ __PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 2 });
    }

    static std::string get_current_timestamp() {
        namespace ch = std::chrono;

        auto time = ch::duration_cast<ch::seconds>(ch::system_clock::now().time_since_epoch()).count();

        std::string buf(128, '\0');
        buf.resize(std::strftime(buf.data(), buf.size(), "%Y-%m-%d %X", std::localtime(&time)));

        return buf;
    }
}

#endif //VKPLAYGROUND_UTIL_HPP
