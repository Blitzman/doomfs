#ifndef VULKAN_APPLICATION_HPP_
#define VULKAN_APPLICATION_HPP_

#include <experimental/optional>
#include <fstream>
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

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR m_capabilities;
    std::vector<VkSurfaceFormatKHR> m_formats;
    std::vector<VkPresentModeKHR> m_present_modes;
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

  const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
        create_swap_chain();
        create_image_views();
        create_graphics_pipeline();
    }

    ~VulkanApplication()
    {
        if (kEnableValidationLayers)
            destroy_debug_utils_messengerext(m_vk_instance, m_callback, nullptr);

        for (unsigned int i = 0; i < m_swap_chain_image_views.size(); ++i)
          vkDestroyImageView(m_device, m_swap_chain_image_views[i], nullptr);

        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
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

    bool check_device_extension_support(VkPhysicalDevice device)
    {
      uint32_t extension_count_;
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count_, nullptr);
      std::vector<VkExtensionProperties> available_extensions_(extension_count_);
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count_, available_extensions_.data());

      std::set<std::string> required_extensions_(kDeviceExtensions.begin(), kDeviceExtensions.end());

      for (const auto& extension : available_extensions_)
        required_extensions_.erase(extension.extensionName);

      return required_extensions_.empty();
    }

    bool is_device_suitable(VkPhysicalDevice device)
    {
      VkPhysicalDeviceProperties device_properties_;
      vkGetPhysicalDeviceProperties(device, &device_properties_);

      // Support for optional features like texture compression, 64-bits floats and multi-viewport
      // rendering can be queried using device features...
      VkPhysicalDeviceFeatures device_features_;
      vkGetPhysicalDeviceFeatures(device, &device_features_);

      // Check device extension support
      bool extensions_supported_ = check_device_extension_support(device);

      // Verify that swap chain support is adequate
      bool swap_chain_adequate_ = false;

      if (extensions_supported_)
      {
        SwapChainSupportDetails swap_chain_support_ = query_swap_chain_support(device);
        swap_chain_adequate_ = !swap_chain_support_.m_formats.empty() && !swap_chain_support_.m_present_modes.empty();
      }

      // Query queue families
      QueueFamilyIndices qf_indices_ = find_queue_families(device);

      // For the moment, we'll settle with just any discrete GPU that supports Vulkan and has the
      // proper queue families.
      return device_properties_.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
              qf_indices_.is_complete() &&
              extensions_supported_ &&
              swap_chain_adequate_;
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

      // Enable device extensions
      device_create_info_.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
      device_create_info_.ppEnabledExtensionNames = kDeviceExtensions.data();

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

    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& crAvailableFormats)
    {
      // assert there are available formats

      if (crAvailableFormats.size() == 1 && crAvailableFormats[0].format == VK_FORMAT_UNDEFINED)
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

      for (const auto& format : crAvailableFormats)
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
          return format;

      return crAvailableFormats[0];
    }

    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& crAvailablePresentModes)
    {
      // Fall back to the only mode guaranteed to be available
      VkPresentModeKHR mode_ = VK_PRESENT_MODE_FIFO_KHR;

      for (const auto& mode : crAvailablePresentModes)
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
          return mode;
        else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
          mode_ = mode;

      return mode_;
    }

    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& crCapabilities)
    {
      if (crCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return crCapabilities.currentExtent;
      else
      {
        int window_width_;
        int window_height_;

        glfwGetWindowSize(*m_window, &window_width_, &window_height_);

        VkExtent2D actual_extent_ = {static_cast<uint32_t>(window_width_), static_cast<uint32_t>(window_height_)};

        actual_extent_.width = std::max(crCapabilities.minImageExtent.width,
                                        std::min(crCapabilities.maxImageExtent.width,
                                            actual_extent_.width));
        actual_extent_.height = std::max(crCapabilities.minImageExtent.height,
                                          std::min(crCapabilities.maxImageExtent.height,
                                            actual_extent_.height));
        return actual_extent_;
      }
    }

    void create_surface()
    {
      if (glfwCreateWindowSurface(m_vk_instance, *m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
    }

    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device)
    {
      // TODO: assert surface
      SwapChainSupportDetails details_;

      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details_.m_capabilities);

      uint32_t format_count_;
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count_, nullptr);

      if (format_count_ != 0)
      {
        details_.m_formats.resize(format_count_);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count_, details_.m_formats.data());
      }

      uint32_t present_mode_count_;
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count_, nullptr);

      if (present_mode_count_ != 0)
      {
        details_.m_present_modes.resize(present_mode_count_);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count_, details_.m_present_modes.data());
      }

      return details_;
    }

    void create_swap_chain()
    {
        SwapChainSupportDetails swap_chain_support_ = query_swap_chain_support(m_physical_device);

        VkSurfaceFormatKHR surface_format_ = choose_swap_surface_format(swap_chain_support_.m_formats);
        VkPresentModeKHR present_mode_ = choose_swap_present_mode(swap_chain_support_.m_present_modes);
        VkExtent2D extent_ = choose_swap_extent(swap_chain_support_.m_capabilities);

        uint32_t image_count_ = swap_chain_support_.m_capabilities.minImageCount + 1;
        if (swap_chain_support_.m_capabilities.maxImageCount > 0 && image_count_ > swap_chain_support_.m_capabilities.maxImageCount)
            image_count_ = swap_chain_support_.m_capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR create_info_ = {};
        create_info_.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info_.surface = m_surface;
        create_info_.minImageCount = image_count_;
        create_info_.imageFormat = surface_format_.format;
        create_info_.imageColorSpace = surface_format_.colorSpace;
        create_info_.imageExtent = extent_;
        create_info_.imageArrayLayers = 1;
        create_info_.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices_ = find_queue_families(m_physical_device);
        uint32_t qf_indices_[] = {
            indices_.m_graphics_family.value(),
            indices_.m_present_family.value()
        };

        if (indices_.m_graphics_family != indices_.m_present_family)
        {
            create_info_.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info_.queueFamilyIndexCount = 2;
            create_info_.pQueueFamilyIndices = qf_indices_;
        }
        else
        {
            create_info_.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info_.queueFamilyIndexCount = 0;
            create_info_.pQueueFamilyIndices = nullptr;
        }

        create_info_.preTransform = swap_chain_support_.m_capabilities.currentTransform;
        create_info_.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info_.presentMode = present_mode_;
        create_info_.clipped = VK_TRUE;
        create_info_.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &create_info_, nullptr, &m_swap_chain) != VK_SUCCESS)
          throw std::runtime_error("Failed to create swap chain!");

        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count_, nullptr);
        m_swap_chain_images.resize(image_count_);
        vkGetSwapchainImagesKHR(m_device, m_swap_chain, &image_count_, m_swap_chain_images.data());

        m_swap_chain_format = surface_format_.format;
        m_swap_chain_extent = extent_;
    }

    void create_image_views()
    {
      m_swap_chain_image_views.resize(m_swap_chain_images.size());

      for (unsigned int i = 0; i < m_swap_chain_images.size(); ++i)
      {
        VkImageViewCreateInfo create_info_ = {};
        create_info_.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info_.image = m_swap_chain_images[i];
        create_info_.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info_.format = m_swap_chain_format;
        create_info_.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info_.subresourceRange.baseMipLevel = 0;
        create_info_.subresourceRange.levelCount = 1;
        create_info_.subresourceRange.baseArrayLayer = 0;
        create_info_.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &create_info_, nullptr, &m_swap_chain_image_views[i]) != VK_SUCCESS)
          throw std::runtime_error("Failed to create image view!");
      }
    }

    std::vector<char> read_file(const std::string& crFilename)
    {
      std::ifstream f_(crFilename, std::ios::ate | std::ios::binary);

      if (!f_.is_open())
        throw std::runtime_error("Failed to open file " + crFilename + "!");

      size_t file_size_ = (size_t)f_.tellg();
      std::vector<char> buffer_(file_size_);

      f_.seekg(0);
      f_.read(buffer_.data(), file_size_);

      f_.close();

      return buffer_;
    }

    VkShaderModule create_shader_module(const std::vector<char>& crCode)
    {
      //TODO: assert device

      VkShaderModuleCreateInfo create_info_ = {};
      create_info_.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      create_info_.codeSize = crCode.size();
      create_info_.pCode = reinterpret_cast<const uint32_t*>(crCode.data());

      VkShaderModule shader_module_;

      if (vkCreateShaderModule(m_device, &create_info_, nullptr, &shader_module_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

      return shader_module_;
    }

    void create_graphics_pipeline()
    {
        //TODO: assert device

        auto vertex_shader_code_ = read_file("shaders/vert.spv");
        auto fragment_shader_code_ = read_file("shaders/frag.spv");

        VkShaderModule vertex_shader_module_ = create_shader_module(vertex_shader_code_);
        VkShaderModule fragment_shader_module_ = create_shader_module(fragment_shader_code_);

        // Shader stage creation
        VkPipelineShaderStageCreateInfo vertex_shader_stage_info_ = {};
        vertex_shader_stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_shader_stage_info_.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_shader_stage_info_.module = vertex_shader_module_;
        vertex_shader_stage_info_.pName = "main";

        VkPipelineShaderStageCreateInfo fragment_shader_stage_info_ = {};
        fragment_shader_stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_shader_stage_info_.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_shader_stage_info_.module = vertex_shader_module_;
        fragment_shader_stage_info_.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages_[] = {
            vertex_shader_stage_info_,
            fragment_shader_stage_info_
        };

        vkDestroyShaderModule(m_device, fragment_shader_module_, nullptr);
        vkDestroyShaderModule(m_device, vertex_shader_module_, nullptr);
    }

    VkInstance m_vk_instance;
    VkDebugUtilsMessengerEXT m_callback;
    VkSurfaceKHR m_surface;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    VkDevice m_device;
    VkQueue m_graphics_queue;
    VkQueue m_present_queue;
    VkSwapchainKHR m_swap_chain;
    std::vector<VkImage> m_swap_chain_images;
    VkFormat m_swap_chain_format;
    VkExtent2D m_swap_chain_extent;
    std::vector<VkImageView> m_swap_chain_image_views;

    std::shared_ptr<GLFWwindow*> m_window;
};

#endif