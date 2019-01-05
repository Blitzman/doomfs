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

    bool check_validation_layer_support(const std::vector<const char*>& crValidationLayers)
    {
      uint32_t layer_count_;
      vkEnumerateInstanceLayerProperties(&layer_count_, nullptr);

      std::vector<VkLayerProperties> av_layer_properties_(layer_count_);
      vkEnumerateInstanceLayerProperties(&layer_count_, av_layer_properties_.data());

      for (const char* l : crValidationLayers)
      {
        bool layer_found_ = false;

        for (const auto& lp : av_layer_properties_)
        {
          if (strcmp(l, lp.layerName) == 0)
          {
            layer_found_ = true;
            break;
          }
        }

        if (!layer_found_)
          return false;
      }

      return true;
    }

    void init_vulkan()
    {
      #ifdef NDEBUG
        const bool kEnableValidationLayers = false;
      #else
        const bool kEnableValidationLayers = true;
      #endif
      
      const std::vector<const char*> validation_layers_ = {
        "VK_LAYER_LUNARG_standard_validation"
      };

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

      if (kEnableValidationLayers)
      {
        if (!check_validation_layer_support(validation_layers_))
          throw std::runtime_error("Validation layers requested, but some are not available!");

        create_info_.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
        create_info_.ppEnabledLayerNames = validation_layers_.data();
      }

      if (vkCreateInstance(&create_info_, nullptr, &m_vk_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vkInstance");
    }

    void init()
    {
      std::cout << "Application initialization...\n";

      const int kWidth = 800;
      const int kHeight = 600;

      init_vulkan();

      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      m_window.reset(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr));
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