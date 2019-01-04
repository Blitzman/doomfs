#ifndef APPLICATION_HPP_
#define APPLICATION_HPP_

#include <cstdlib>
#include <iostream>
#include <memory>

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
      glfwInit();

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      m_window.reset(glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr));

      uint32_t extension_count_ = 0;
	    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count_, nullptr);

	    std::cout << extension_count_ << " extensions supported\n";
    }

    void loop()
    {
	    while (!glfwWindowShouldClose(m_window.get()))
		    glfwPollEvents();
    }

    void cleanup()
    {
      m_window.reset();
	    glfwTerminate();
    }

    std::unique_ptr<GLFWwindow, glfwDeleter> m_window;
};


#endif