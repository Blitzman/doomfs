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

      glfwInit();
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
      m_window = std::make_shared<GLFWwindow*>(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr));

      m_vulkan = std::make_unique<VulkanApplication>(m_window);
    }

    void loop()
    {
      std::cout << "Application loop...\n";

	    while (!glfwWindowShouldClose(*m_window))
		    glfwPollEvents();
    }

    void cleanup()
    {
      std::cout << "Application cleanup...\n";

      m_vulkan.reset();
      std::cout << "Cleaned Vulkan application...\n";

      std::cout << "Destroying GLFW Window Context" << std::endl;
      glfwDestroyWindow(*m_window);

      m_window.reset();
	    glfwTerminate();
      std::cout << "Cleaned GLFW window...\n";
    }

    std::shared_ptr<GLFWwindow*> m_window;
    std::unique_ptr<VulkanApplication> m_vulkan;
};


#endif