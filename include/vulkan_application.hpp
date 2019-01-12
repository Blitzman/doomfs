#ifndef VULKAN_APPLICATION_HPP_
#define VULKAN_APPLICATION_HPP_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_NONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

VkResult create_debug_utils_messengerext(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
      VkDebugUtilsMessengerEXT* pCallback)
{
  auto function_ = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

  if (function_ != nullptr)
    return function_(instance, pCreateInfo, pAllocator, pCallback);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroy_debug_utils_messengerext(
  VkInstance instance,
  VkDebugUtilsMessengerEXT callback,
  const VkAllocationCallbacks* pAllocator)
{
  auto function_ = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

  if (function_ != nullptr)
    function_(instance, callback, pAllocator);
}

class VulkanApplication
{
  #ifdef NDEBUG
    const bool kEnableValidationLayers = false;
  #else
    const bool kEnableValidationLayers = true;
  #endif

  public:

    VulkanApplication()
    {
      init_vulkan();
    }

    ~VulkanApplication()
    {
      if (kEnableValidationLayers)
        destroy_debug_utils_messengerext(m_vk_instance, m_callback, nullptr);

      vkDestroyInstance(m_vk_instance, nullptr);
    }

  private:

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
      std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n";
      return VK_FALSE;
    }

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

    std::vector<const char *> get_required_extensions(bool enableValidation)
    {
      uint32_t glfw_extension_count_ = 0;
      const char** glfw_extensions_;

      glfw_extensions_ = glfwGetRequiredInstanceExtensions(&glfw_extension_count_);

      std::vector<const char*> extensions_(glfw_extensions_, glfw_extensions_ + glfw_extension_count_);

      if (kEnableValidationLayers)
        extensions_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

      return extensions_;
    }

    void setup_debug_callback()
    {
      if (!kEnableValidationLayers)
        return;

      VkDebugUtilsMessengerCreateInfoEXT create_info_ = {};
      create_info_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      create_info_.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      create_info_.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      create_info_.pfnUserCallback = debug_callback;
      create_info_.pUserData = nullptr; // Optional

      if (create_debug_utils_messengerext(m_vk_instance, &create_info_, nullptr, &m_callback) != VK_SUCCESS)
        throw std::runtime_error("failed to set up debug callback!");
    }

    void init_vulkan()
    { 
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

      std::vector<const char*> r_extensions_ = get_required_extensions(kEnableValidationLayers);
      create_info_.enabledExtensionCount = static_cast<uint32_t>(r_extensions_.size());
      create_info_.ppEnabledExtensionNames = r_extensions_.data();

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

      setup_debug_callback();
    }

    VkInstance m_vk_instance;
    VkDebugUtilsMessengerEXT m_callback;
};

#endif