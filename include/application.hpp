#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string.h>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_application.hpp"

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

      const int kWidth = 800;
      const int kHeight = 600;

      m_vulkan = std::make_unique<VulkanApplication>();

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

      m_vulkan.reset();

      m_window.reset();
	    glfwTerminate();
    }

    std::unique_ptr<GLFWwindow, glfwDeleter> m_window;
    std::unique_ptr<VulkanApplication> m_vulkan;
};


#endif
