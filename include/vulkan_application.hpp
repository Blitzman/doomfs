#ifndef VULKAN_APPLICATION_HPP_
#define VULKAN_APPLICATION_HPP_

#include <experimental/optional>
#include <set>

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

struct QueueFamilyIndices
{
    std::experimental::optional<uint32_t> m_graphics_family;
    std::experimental::optional<uint32_t> m_present_family;

    bool is_complete()
    {
        return bool(m_graphics_family) && bool(m_present_family);
    }
};

class VulkanApplication
{
  #ifdef NDEBUG
    const bool kEnableValidationLayers = false;
  #else
    const bool kEnableValidationLayers = true;
  #endif

  const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
  };

  public:

    VulkanApplication(std::shared_ptr<GLFWwindow*> pWindow)
    {
      m_window = pWindow;

      init_vulkan();
      setup_debug_callback();
      create_surface();
      pick_physical_device();
      create_logical_device();
    }

    ~VulkanApplication()
    {
      if (kEnableValidationLayers)
        destroy_debug_utils_messengerext(m_vk_instance, m_callback, nullptr);

      vkDestroyDevice(m_device, nullptr);
      vkDestroySurfaceKHR(m_vk_instance, m_surface, nullptr);
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
        if (!check_validation_layer_support(kValidationLayers))
          throw std::runtime_error("Validation layers requested, but some are not available!");

        create_info_.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        create_info_.ppEnabledLayerNames = kValidationLayers.data();
      }

      if (vkCreateInstance(&create_info_, nullptr, &m_vk_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vkInstance");
    }

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device)
    {
      QueueFamilyIndices qf_indices_;

      uint32_t qf_count_ = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(device, &qf_count_, nullptr);
      std::vector<VkQueueFamilyProperties> qf_properties_(qf_count_);
      vkGetPhysicalDeviceQueueFamilyProperties(device, &qf_count_, qf_properties_.data());

      int i = 0;
      for (const auto& qf : qf_properties_)
      {
        if (qf.queueCount > 0 && qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
          qf_indices_.m_graphics_family = i;

        VkBool32 present_support_ = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &present_support_);

        if (qf.queueCount > 0 && present_support_)
          qf_indices_.m_present_family = i;

        if (qf_indices_.is_complete())
          break;
        
        ++i;
      }

      return qf_indices_;
    }

    bool is_device_suitable(VkPhysicalDevice device)
    {
      VkPhysicalDeviceProperties device_properties_;
      vkGetPhysicalDeviceProperties(device, &device_properties_);

      // Support for optional features like texture compression, 64-bits floats and multi-viewport
      // rendering can be queried using device features...
      VkPhysicalDeviceFeatures device_features_;
      vkGetPhysicalDeviceFeatures(device, &device_features_);

      QueueFamilyIndices qf_indices_ = find_queue_families(device);

      // For the moment, we'll settle with just any discrete GPU that supports Vulkan and has the
      // proper queue families.
      return device_properties_.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
              qf_indices_.is_complete();
    }

    void pick_physical_device()
    {
      uint32_t device_count_ = 0;
      vkEnumeratePhysicalDevices(m_vk_instance, &device_count_, nullptr);

      if (device_count_ == 0)
       throw std::runtime_error("failed to find GPUs with Vulkan support!");

      std::vector<VkPhysicalDevice> devices_(device_count_);
      vkEnumeratePhysicalDevices(m_vk_instance, &device_count_, devices_.data());

      for (const auto& device : devices_)
      {
        if (is_device_suitable(device)) {
          m_physical_device = device;
          break;
        }
      }

      if (m_physical_device == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    void create_logical_device()
    {
      QueueFamilyIndices qf_indices_ = find_queue_families(m_physical_device);

      std::vector<VkDeviceQueueCreateInfo> queue_create_infos_;
      std::set<uint32_t> unique_queue_families_ = {
        qf_indices_.m_graphics_family.value(),
        qf_indices_.m_present_family.value()
      };

      float queue_priority_ = 1.0f;

      for (uint32_t qf : unique_queue_families_)
      {
        VkDeviceQueueCreateInfo queue_create_info_ = {};
        queue_create_info_.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info_.queueFamilyIndex = qf;
        queue_create_info_.queueCount = 1;
        queue_create_info_.pQueuePriorities = & queue_priority_;
        queue_create_infos_.push_back(queue_create_info_);
      }

      VkPhysicalDeviceFeatures device_features_ = {};

      VkDeviceCreateInfo device_create_info_ = {};
      device_create_info_.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      device_create_info_.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos_.size());
      device_create_info_.pQueueCreateInfos = queue_create_infos_.data();
      device_create_info_.pEnabledFeatures = &device_features_;

      device_create_info_.enabledExtensionCount = 0;

      if (kEnableValidationLayers)
      {
        device_create_info_.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
        device_create_info_.ppEnabledLayerNames = kValidationLayers.data();
      }
      else
        device_create_info_.enabledLayerCount = 0;

      if (vkCreateDevice(m_physical_device, &device_create_info_, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

      vkGetDeviceQueue(m_device, qf_indices_.m_graphics_family.value(), 0, &m_graphics_queue);
      vkGetDeviceQueue(m_device, qf_indices_.m_present_family.value(), 0, &m_present_queue);
    }

    void create_surface()
    {
      if (glfwCreateWindowSurface(m_vk_instance, *m_window.get(), nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
    }

    VkInstance m_vk_instance;
    VkDebugUtilsMessengerEXT m_callback;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphics_queue;
    VkQueue m_present_queue;

    std::shared_ptr<GLFWwindow*> m_window;
};

#endif