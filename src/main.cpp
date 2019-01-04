#include <cstdint>
#include <fstream>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_NONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "wad.hpp"

#define WAD_FILENAME "doom1.wad"

struct glfwDeleter
{
    void operator()(GLFWwindow *wnd)
    {
        std::cout << "Destroying GLFW Window Context" << std::endl;
        glfwDestroyWindow(wnd);
    }
};

int main(void)
{
	WAD wad_(WAD_FILENAME);
	std::cout << wad_;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	std::unique_ptr<GLFWwindow, glfwDeleter> window_;
	window_.reset(glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr));

	uint32_t extension_count_ = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count_, nullptr);

	std::cout << extension_count_ << " extensions supported\n";

	glm::mat4 matrix_;
	glm::vec4 vec_;
	auto test_ = matrix_ * vec_;

	while (!glfwWindowShouldClose(window_.get()))
		glfwPollEvents();

	window_.reset();
	glfwTerminate();

	return 0;
}
