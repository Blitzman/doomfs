#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_NONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

struct glfwDeleter
{
  void operator()(GLFWwindow *wnd)
  {
      std::cout << "Destroying GLFW Window Context" << std::endl;
      glfwDestroyWindow(wnd);
  }
};

class Application
{
  public:

    Application()
    {
      
    }

    void run()
    {
      init();
      loop();
      cleanup();
    }

  private:

    void init()
    {
      std::cout << "Application initialization...\n";

      glfwInit();

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      m_window.reset(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr));

      uint32_t extension_count_ = 0;
	    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count_, nullptr);
      std::vector<VkExtensionProperties> extensions_(extension_count_);
      vkEnumerateInstanceExtensionProperties(nullptr, &extension_count_, extensions_.data());

      std::cout << extension_count_ << " extensions supported\n";
      for (const auto& e : extensions_)
        std::cout << "\t" << e.extensionName << "\n";

      VkApplicationInfo app_info_ = {};
      app_info_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      app_info_.pApplicationName = "DOOMFS";
      app_info_.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info_.pEngineName = "No Engine";
      app_info_.engineVersion = VK_MAKE_VERSION(1, 0, 0);
      app_info_.apiVersion = VK_API_VERSION_1_0;

      VkInstanceCreateInfo create_info_ = {};
      create_info_.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      create_info_.pApplicationInfo = &app_info_;

      uint32_t glfw_extension_count_ = 0;
      const char** glfw_extensions_;

      glfw_extensions_ = glfwGetRequiredInstanceExtensions(&glfw_extension_count_);

      create_info_.enabledExtensionCount = glfw_extension_count_;
      create_info_.ppEnabledExtensionNames = glfw_extensions_;
      create_info_.enabledLayerCount = 0;

      if (vkCreateInstance(&create_info_, nullptr, &m_vk_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vkInstance");
    }

    void loop()
    {
      std::cout << "Application loop...\n";

	    while (!glfwWindowShouldClose(m_window.get()))
		    glfwPollEvents();
    }

    void cleanup()
    {
      std::cout << "Application cleanup...\n";

      vkDestroyInstance(m_vk_instance, nullptr);

      m_window.reset();
	    glfwTerminate();
    }

    std::unique_ptr<GLFWwindow, glfwDeleter> m_window;
    VkInstance m_vk_instance;

};


#endif