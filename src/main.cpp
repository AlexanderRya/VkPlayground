#include <iostream>
#include "application.hpp"

int main() {
    vk_playground::application app{};
    app.glfw_init();
    app.vk_init();
    //eng.run();
    return 0;
}
