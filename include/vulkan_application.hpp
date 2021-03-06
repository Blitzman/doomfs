#ifndef VULKAN_APPLICATION_HPP_
#define VULKAN_APPLICATION_HPP_

#include <chrono>
#include <experimental/optional>
#include <fstream>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct UniformBufferObject
{
    alignas(16) glm::mat4 m_model;
    alignas(16) glm::mat4 m_view;
    alignas(16) glm::mat4 m_proj;
};

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

  const int kMaxFramesInFlight = 2;

  const std::vector<const char*> kValidationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
  };

  const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  const std::vector<Vertex> kVertices =
  {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
  };
  const std::vector<uint16_t> kVertexIndices = 
  {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
  };

  public:

    VulkanApplication(std::shared_ptr<GLFWwindow*> pWindow)
    {
        m_window = pWindow;
        glfwSetWindowUserPointer(*m_window, this);
        glfwSetFramebufferSizeCallback(*m_window, framebuffer_resize_callback);

        m_current_frame = 0;
        m_framebuffer_resized = false;

        init_vulkan();
        setup_debug_callback();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_descriptorset_layout();
        create_graphics_pipeline();
        create_framebuffers();
        create_commandpool();
        create_depth_resources();
        create_textureimage();
        create_textureimageview();
        create_texturesampler();
        create_vertexbuffer();
        create_indexbuffer();
        create_uniformbuffers();
        create_descriptorpool();
        create_descriptorsets();
        create_commandbuffers();
        create_semaphores();
        create_fences();
    }

    ~VulkanApplication()
    {
        cleanup_swapchain();

        vkDestroySampler(m_device, m_texture_sampler, nullptr);
        vkDestroyImageView(m_device, m_texture_image_view, nullptr);

        vkDestroyImage(m_device, m_texture_image, nullptr);
        vkFreeMemory(m_device, m_texture_image_memory, nullptr);

        vkDestroyDescriptorPool(m_device, m_descriptorpool, nullptr);

        vkDestroyDescriptorSetLayout(m_device, m_descriptorset_layout, nullptr);

        for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
        {
            vkDestroyBuffer(m_device, m_uniform_buffers[i], nullptr);
            vkFreeMemory(m_device, m_uniformbuffers_memory[i], nullptr);
        }

        vkDestroyBuffer(m_device, m_indexbuffer, nullptr);
        vkFreeMemory(m_device, m_indexbuffer_memory, nullptr);

        vkDestroyBuffer(m_device, m_vertexbuffer, nullptr);
        vkFreeMemory(m_device, m_vertexbuffer_memory, nullptr);

        for (size_t i = 0; i < kMaxFramesInFlight; ++i)
            vkDestroyFence(m_device, m_inflight_fences[i], nullptr);

        for (size_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_commandpool, nullptr);
        
        vkDestroyDevice(m_device, nullptr);

        if (kEnableValidationLayers)
            destroy_debug_utils_messengerext(m_vk_instance, m_callback, nullptr);

        vkDestroySurfaceKHR(m_vk_instance, m_surface, nullptr);
        vkDestroyInstance(m_vk_instance, nullptr);
    }

    void draw_frame()
    {
        uint32_t image_index_;

        vkWaitForFences(m_device, 1, &m_inflight_fences[m_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

        VkResult result_ = vkAcquireNextImageKHR(m_device,
                                                 m_swap_chain,
                                                 std::numeric_limits<uint64_t>::max(),
                                                 m_image_available_semaphores[m_current_frame],
                                                 VK_NULL_HANDLE,
                                                 &image_index_);

        if (result_ == VK_ERROR_OUT_OF_DATE_KHR)
        {
            recreate_swapchain();
            return;
        }
        else if (result_ != VK_SUCCESS && result_ != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image!");

        update_uniformbuffer(image_index_);

        VkSubmitInfo submit_info_ = {};
        submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores_[] = {m_image_available_semaphores[m_current_frame]};
        VkPipelineStageFlags wait_stages_[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submit_info_.waitSemaphoreCount = 1;
        submit_info_.pWaitSemaphores = wait_semaphores_;
        submit_info_.pWaitDstStageMask = wait_stages_;

        submit_info_.commandBufferCount = 1;
        submit_info_.pCommandBuffers = &m_commandbuffers[image_index_];

        VkSemaphore signal_semaphores_[] = {m_render_finished_semaphores[m_current_frame]};

        submit_info_.signalSemaphoreCount = 1;
        submit_info_.pSignalSemaphores = signal_semaphores_;

        vkResetFences(m_device, 1, &m_inflight_fences[m_current_frame]);

        if (vkQueueSubmit(m_graphics_queue, 1, &submit_info_, m_inflight_fences[m_current_frame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command buffer!");

        VkPresentInfoKHR present_info_ = {};
        present_info_.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info_.waitSemaphoreCount = 1;
        present_info_.pWaitSemaphores = signal_semaphores_;

        VkSwapchainKHR swap_chains_[] = { m_swap_chain };
        present_info_.swapchainCount = 1;
        present_info_.pSwapchains = swap_chains_;
        present_info_.pImageIndices = &image_index_;

        present_info_.pResults = nullptr;

        result_ = vkQueuePresentKHR(m_present_queue, &present_info_);

        if (result_ == VK_ERROR_OUT_OF_DATE_KHR || result_ == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
        {
            m_framebuffer_resized = false;
            recreate_swapchain();
        }
        else if (result_ != VK_SUCCESS)
            throw std::runtime_error("Failed to present swap chain image!");

        vkQueueWaitIdle(m_present_queue);

        m_current_frame = (m_current_frame + 1) % kMaxFramesInFlight;
    }

    void wait_device()
    {
        vkDeviceWaitIdle(m_device);
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
              swap_chain_adequate_ &&
              device_features_.samplerAnisotropy;
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
      device_features_.samplerAnisotropy = VK_TRUE;

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

        glfwGetFramebufferSize(*m_window, &window_width_, &window_height_);

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
            m_swap_chain_image_views[i] = create_image_view(m_swap_chain_images[i], m_swap_chain_format, VK_IMAGE_ASPECT_COLOR_BIT);
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
        fragment_shader_stage_info_.module = fragment_shader_module_;
        fragment_shader_stage_info_.pName = "main";

        VkPipelineShaderStageCreateInfo shader_stages_[] = {
            vertex_shader_stage_info_,
            fragment_shader_stage_info_
        };

        // Vertex input
        auto binding_description_ = Vertex::get_binding_description();
        auto attribute_descriptions_ = Vertex::get_attribute_descriptions();

        VkPipelineVertexInputStateCreateInfo vertex_input_info_ = {};
        vertex_input_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info_.vertexBindingDescriptionCount = 1;
        vertex_input_info_.pVertexBindingDescriptions = &binding_description_;
        vertex_input_info_.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions_.size());
        vertex_input_info_.pVertexAttributeDescriptions = attribute_descriptions_.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo input_assembly_info_ = {};
        input_assembly_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_info_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_info_.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport_ = {};
        viewport_.x = 0.0f;
        viewport_.y = 0.0f;
        viewport_.width = (float)m_swap_chain_extent.width;
        viewport_.height = (float)m_swap_chain_extent.height;
        viewport_.minDepth = 0.0f;
        viewport_.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor_ = {};
        scissor_.offset = {0, 0};
        scissor_.extent = m_swap_chain_extent;

        VkPipelineViewportStateCreateInfo viewport_state_info_ = {};
        viewport_state_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_info_.viewportCount = 1;
        viewport_state_info_.pViewports = &viewport_;
        viewport_state_info_.scissorCount = 1;
        viewport_state_info_.pScissors = &scissor_;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer_info_ = {};
        rasterizer_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer_info_.depthClampEnable = VK_FALSE;
        rasterizer_info_.rasterizerDiscardEnable = VK_FALSE;
        rasterizer_info_.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer_info_.lineWidth = 1.0f;
        rasterizer_info_.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer_info_.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer_info_.depthBiasEnable = VK_FALSE;
        rasterizer_info_.depthBiasConstantFactor = 0.0f;
        rasterizer_info_.depthBiasClamp = 0.0f;
        rasterizer_info_.depthBiasSlopeFactor = 0.0f;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling_info_ = {};
        multisampling_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling_info_.sampleShadingEnable = VK_FALSE;
        multisampling_info_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling_info_.minSampleShading = 1.0f;
        multisampling_info_.pSampleMask = nullptr;
        multisampling_info_.alphaToCoverageEnable = VK_FALSE;

        // Color blending
        VkPipelineColorBlendAttachmentState colorblend_attachment_ = {};
        colorblend_attachment_.colorWriteMask = 
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorblend_attachment_.blendEnable = VK_TRUE;
        colorblend_attachment_.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorblend_attachment_.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorblend_attachment_.colorBlendOp = VK_BLEND_OP_ADD;
        colorblend_attachment_.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorblend_attachment_.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorblend_attachment_.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorblend_info_ = {};
        colorblend_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorblend_info_.logicOpEnable = VK_FALSE;
        colorblend_info_.logicOp = VK_LOGIC_OP_COPY;
        colorblend_info_.attachmentCount = 1;
        colorblend_info_.pAttachments = &colorblend_attachment_;
        colorblend_info_.blendConstants[0] = 0.0f;
        colorblend_info_.blendConstants[1] = 0.0f;
        colorblend_info_.blendConstants[2] = 0.0f;
        colorblend_info_.blendConstants[3] = 0.0f;

        // Dynamic state
        VkDynamicState dynamic_states_[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicstate_info_ = {};
        dynamicstate_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicstate_info_.dynamicStateCount = 2;
        dynamicstate_info_.pDynamicStates = dynamic_states_;

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelinelayout_info_ = {};
        pipelinelayout_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelinelayout_info_.setLayoutCount = 1;
        pipelinelayout_info_.pSetLayouts = &m_descriptorset_layout;
        pipelinelayout_info_.pushConstantRangeCount = 0;
        pipelinelayout_info_.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(m_device, &pipelinelayout_info_, nullptr, &m_pipeline_layout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");

        VkGraphicsPipelineCreateInfo pipeline_info_ = {};
        pipeline_info_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info_.stageCount = 2;
        pipeline_info_.pStages = shader_stages_;
        pipeline_info_.pVertexInputState = &vertex_input_info_;
        pipeline_info_.pInputAssemblyState = &input_assembly_info_;
        pipeline_info_.pViewportState = &viewport_state_info_;
        pipeline_info_.pRasterizationState = &rasterizer_info_;
        pipeline_info_.pMultisampleState = &multisampling_info_;
        pipeline_info_.pDepthStencilState = nullptr;
        pipeline_info_.pColorBlendState = &colorblend_info_;
        pipeline_info_.pDynamicState = nullptr;
        pipeline_info_.layout = m_pipeline_layout;
        pipeline_info_.renderPass = m_render_pass;
        pipeline_info_.subpass = 0;
        pipeline_info_.basePipelineHandle = VK_NULL_HANDLE;
        pipeline_info_.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info_, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
          throw std::runtime_error("Failed to create graphics pipeline!");

        vkDestroyShaderModule(m_device, fragment_shader_module_, nullptr);
        vkDestroyShaderModule(m_device, vertex_shader_module_, nullptr);
    }

    void create_render_pass()
    {
        VkAttachmentDescription color_attachment_ = {};
        color_attachment_.format = m_swap_chain_format;
        color_attachment_.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment_.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment_.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment_.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment_.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depth_attachment_ = {};
        depth_attachment_.format = find_depth_format();
        depth_attachment_.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment_.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment_.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment_.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment_.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_attachment_ref_ = {};
        color_attachment_ref_.attachment = 0;
        color_attachment_ref_.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref_ = {};
        depth_attachment_ref_.attachment = 1;
        depth_attachment_ref_.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_ = {};
        subpass_.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_.colorAttachmentCount = 1;
        subpass_.pColorAttachments = &color_attachment_ref_;
        subpass_.pDepthStencilAttachment = &depth_attachment_ref_;

        VkSubpassDependency dependency_ = {};
        dependency_.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency_.dstSubpass = 0;
        dependency_.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency_.srcAccessMask = 0;
        dependency_.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency_.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments_ = {color_attachment_, depth_attachment_};
        VkRenderPassCreateInfo render_pass_info_ = {};
        render_pass_info_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info_.attachmentCount = static_cast<uint32_t>(attachments_.size());
        render_pass_info_.pAttachments = attachments_.data();
        render_pass_info_.subpassCount = 1;
        render_pass_info_.pSubpasses = &subpass_;
        render_pass_info_.dependencyCount = 1;
        render_pass_info_.pDependencies = &dependency_;
        
        if (vkCreateRenderPass(m_device, &render_pass_info_, nullptr, &m_render_pass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }

    void create_framebuffers()
    {
        m_swap_chain_framebuffers.resize(m_swap_chain_image_views.size());

        for (size_t i = 0; i < m_swap_chain_image_views.size(); ++i)
        {
            VkImageView attachments_[] = {
                m_swap_chain_image_views[i]
            };

            VkFramebufferCreateInfo framebuffer_info_ = {};
            framebuffer_info_.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info_.renderPass = m_render_pass;
            framebuffer_info_.attachmentCount = 1;
            framebuffer_info_.pAttachments = attachments_;
            framebuffer_info_.width = m_swap_chain_extent.width;
            framebuffer_info_.height = m_swap_chain_extent.height;
            framebuffer_info_.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebuffer_info_, nullptr, &m_swap_chain_framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    void create_commandpool()
    {
        QueueFamilyIndices qf_indices_ = find_queue_families(m_physical_device);

        VkCommandPoolCreateInfo pool_info_ = {};
        pool_info_.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info_.queueFamilyIndex = qf_indices_.m_graphics_family.value();
        pool_info_.flags = 0;

        if (vkCreateCommandPool(m_device, &pool_info_, nullptr, &m_commandpool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }

    void create_commandbuffers()
    {
        m_commandbuffers.resize(m_swap_chain_framebuffers.size());

        VkCommandBufferAllocateInfo alloc_info_ = {};
        alloc_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info_.commandPool = m_commandpool;
        alloc_info_.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info_.commandBufferCount = (uint32_t) m_commandbuffers.size();

        if (vkAllocateCommandBuffers(m_device, &alloc_info_, m_commandbuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");

        for (size_t i = 0; i < m_commandbuffers.size(); ++i)
        {
          VkCommandBufferBeginInfo begin_info_ = {};
          begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
          begin_info_.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
          begin_info_.pInheritanceInfo = nullptr;

          if (vkBeginCommandBuffer(m_commandbuffers[i], &begin_info_) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");

          VkRenderPassBeginInfo renderpass_info_ = {};
          renderpass_info_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
          renderpass_info_.renderPass = m_render_pass;
          renderpass_info_.framebuffer = m_swap_chain_framebuffers[i];

          renderpass_info_.renderArea.offset = {0, 0};
          renderpass_info_.renderArea.extent = m_swap_chain_extent;

          VkClearValue clear_color_ = {0.0f, 0.0f, 0.0f, 1.0f};
          renderpass_info_.clearValueCount = 1;
          renderpass_info_.pClearValues = &clear_color_;

          vkCmdBeginRenderPass(m_commandbuffers[i], &renderpass_info_, VK_SUBPASS_CONTENTS_INLINE);

          vkCmdBindPipeline(m_commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

          VkBuffer vertex_buffers_[] = { m_vertexbuffer };
          VkDeviceSize offsets_[] = { 0 };
          vkCmdBindVertexBuffers(m_commandbuffers[i], 0, 1, vertex_buffers_, offsets_);
          vkCmdBindIndexBuffer(m_commandbuffers[i], m_indexbuffer, 0, VK_INDEX_TYPE_UINT16);

          vkCmdBindDescriptorSets(m_commandbuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptorsets[i], 0, nullptr);

          //vkCmdDraw(m_commandbuffers[i], static_cast<uint32_t>(kVertices.size()), 1, 0, 0);
          vkCmdDrawIndexed(m_commandbuffers[i], static_cast<uint32_t>(kVertexIndices.size()), 1, 0, 0, 0);

          vkCmdEndRenderPass(m_commandbuffers[i]);

          if (vkEndCommandBuffer(m_commandbuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void create_semaphores()
    {
        m_image_available_semaphores.resize(kMaxFramesInFlight);
        m_render_finished_semaphores.resize(kMaxFramesInFlight);

        VkSemaphoreCreateInfo semaphore_info_ = {};
        semaphore_info_.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            if (vkCreateSemaphore(m_device, &semaphore_info_, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphore_info_, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS)
                    throw std::runtime_error("Failed to create semaphores!");
        }
    }

    void create_fences()
    {
        m_inflight_fences.resize(kMaxFramesInFlight);

        VkFenceCreateInfo fence_info_ = {};
        fence_info_.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info_.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < kMaxFramesInFlight; ++i)
            if (vkCreateFence(m_device, &fence_info_, nullptr, &m_inflight_fences[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create fence!");
    }

    void cleanup_swapchain()
    {
        for (auto fb : m_swap_chain_framebuffers)
            vkDestroyFramebuffer(m_device, fb, nullptr);

        vkFreeCommandBuffers(m_device, m_commandpool, static_cast<uint32_t>(m_commandbuffers.size()), m_commandbuffers.data());

        vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        vkDestroyRenderPass(m_device, m_render_pass, nullptr);

        for (unsigned int i = 0; i < m_swap_chain_image_views.size(); ++i)
          vkDestroyImageView(m_device, m_swap_chain_image_views[i], nullptr);

        vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
    }

    void recreate_swapchain()
    {
        int width_ = 0;
        int height_ = 0;

        while (width_ == 0 || height_ == 0)
        {
            glfwGetFramebufferSize(*m_window, &width_, &height_);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device);

        cleanup_swapchain();

        create_swap_chain();
        create_image_views();
        create_render_pass();
        create_graphics_pipeline();
        create_framebuffers();
        create_commandbuffers();
    }

    static void framebuffer_resize_callback(GLFWwindow* rWindow, int width, int height)
    {
        auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(rWindow));
        app->m_framebuffer_resized = true;
    }

    uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
      VkPhysicalDeviceMemoryProperties mem_properties_;
      vkGetPhysicalDeviceMemoryProperties(m_physical_device, &mem_properties_);

      for (uint32_t i = 0; i < mem_properties_.memoryTypeCount; ++i)
        if ((typeFilter & (1 << i)) &&
            ((mem_properties_.memoryTypes[i].propertyFlags & properties) == properties))
          return i;

      throw std::runtime_error("Failed to find suitable memory type!");
    }

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & rBuffer, VkDeviceMemory & rBufferMemory)
    {
      VkBufferCreateInfo buffer_info_ = {};
      buffer_info_.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_info_.size = size;
      buffer_info_.usage = usage;
      buffer_info_.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if (vkCreateBuffer(m_device, &buffer_info_, nullptr, &rBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create vertex buffer!");

      VkMemoryRequirements mem_requirements_;
      vkGetBufferMemoryRequirements(m_device, rBuffer, &mem_requirements_);

      VkMemoryAllocateInfo alloc_info_ = {};
      alloc_info_.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info_.allocationSize = mem_requirements_.size;
      alloc_info_.memoryTypeIndex = find_memory_type(mem_requirements_.memoryTypeBits, properties);

      if (vkAllocateMemory(m_device, &alloc_info_, nullptr, &rBufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate vertex buffer memory!");

      vkBindBufferMemory(m_device, rBuffer, rBufferMemory, 0);
    }

    void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer command_buffer_ = begin_single_time_commands();

        VkBufferCopy copy_region_ = {};
        copy_region_.srcOffset = 0;
        copy_region_.dstOffset = 0; // Optional
        copy_region_.size = size;

        vkCmdCopyBuffer(command_buffer_, srcBuffer, dstBuffer, 1, &copy_region_);

        end_single_time_commands(command_buffer_);
    }

    void create_vertexbuffer()
    {
      VkDeviceSize buffer_size_ = sizeof(kVertices[0]) * kVertices.size();

      VkBuffer staging_buffer_;
      VkDeviceMemory staging_buffer_memory_;
      create_buffer(buffer_size_,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging_buffer_,
                    staging_buffer_memory_);

      void* data_;
      vkMapMemory(m_device, staging_buffer_memory_, 0, buffer_size_, 0, &data_);
      memcpy(data_, kVertices.data(), (size_t)buffer_size_);
      vkUnmapMemory(m_device, staging_buffer_memory_);

      create_buffer(buffer_size_,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_vertexbuffer, m_vertexbuffer_memory);

      copy_buffer(staging_buffer_, m_vertexbuffer, buffer_size_);

      vkDestroyBuffer(m_device, staging_buffer_, nullptr);
      vkFreeMemory(m_device, staging_buffer_memory_, nullptr);
    }

    void create_indexbuffer()
    {
      VkDeviceSize buffer_size_ = sizeof(kVertexIndices[0]) * kVertexIndices.size();

      VkBuffer staging_buffer_;
      VkDeviceMemory staging_buffer_memory_;
      create_buffer(buffer_size_,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging_buffer_,
                    staging_buffer_memory_);

      void* data_;
      vkMapMemory(m_device, staging_buffer_memory_, 0, buffer_size_, 0, &data_);
      memcpy(data_, kVertexIndices.data(), (size_t)buffer_size_);
      vkUnmapMemory(m_device, staging_buffer_memory_);

      create_buffer(buffer_size_,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    m_indexbuffer, m_indexbuffer_memory);

      copy_buffer(staging_buffer_, m_indexbuffer, buffer_size_);

      vkDestroyBuffer(m_device, staging_buffer_, nullptr);
      vkFreeMemory(m_device, staging_buffer_memory_, nullptr);
    }

    void create_descriptorset_layout()
    {
      VkDescriptorSetLayoutBinding ubo_layout_binding_ = {};
      ubo_layout_binding_.binding = 0;
      ubo_layout_binding_.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      ubo_layout_binding_.descriptorCount = 1;
      ubo_layout_binding_.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
      ubo_layout_binding_.pImmutableSamplers = nullptr;

      VkDescriptorSetLayoutBinding sampler_layout_binding_ = {};
      sampler_layout_binding_.binding = 1;
      sampler_layout_binding_.descriptorCount = 1;
      sampler_layout_binding_.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      sampler_layout_binding_.pImmutableSamplers = nullptr;
      sampler_layout_binding_.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      std::array<VkDescriptorSetLayoutBinding, 2> bindings_ = { ubo_layout_binding_, sampler_layout_binding_ };

      VkDescriptorSetLayoutCreateInfo layout_info_ = {};
      layout_info_.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layout_info_.bindingCount = static_cast<uint32_t>(bindings_.size());
      layout_info_.pBindings = bindings_.data();

      if (vkCreateDescriptorSetLayout(m_device, &layout_info_, nullptr, &m_descriptorset_layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    void create_uniformbuffers()
    {
        VkDeviceSize buffer_size_ = sizeof(UniformBufferObject);

        m_uniform_buffers.resize(m_swap_chain_images.size());
        m_uniformbuffers_memory.resize(m_swap_chain_images.size());

        for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
            create_buffer(buffer_size_,
                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_uniform_buffers[i],
                          m_uniformbuffers_memory[i]);
    }

    void update_uniformbuffer(uint32_t currentImage)
    {
      static auto start_time_ = std::chrono::high_resolution_clock::now();

      auto current_time_ = std::chrono::high_resolution_clock::now();
      float time_ = std::chrono::duration<float, std::chrono::seconds::period>(current_time_ - start_time_).count();

      UniformBufferObject ubo_ = {};
      
      ubo_.m_model = glm::rotate(glm::mat4(1.0f),
                                 time_ * glm::radians(90.0f),
                                 glm::vec3(0.0f, 0.0f, 1.0f));

      ubo_.m_view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));

      ubo_.m_proj = glm::perspective(glm::radians(45.0f),
                                     m_swap_chain_extent.width / (float)m_swap_chain_extent.height,
                                     0.1f,
                                     10.0f);

      ubo_.m_proj[1][1] *= -1;

      void* data_;

      vkMapMemory(m_device, m_uniformbuffers_memory[currentImage], 0, sizeof(ubo_), 0, &data_);
      memcpy(data_, &ubo_, sizeof(ubo_));
      vkUnmapMemory(m_device, m_uniformbuffers_memory[currentImage]);
    }

    void create_descriptorpool()
    {
        std::array<VkDescriptorPoolSize, 2> pool_sizes_ = {};
        pool_sizes_[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes_[0].descriptorCount = static_cast<uint32_t>(m_swap_chain_images.size());
        pool_sizes_[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes_[1].descriptorCount = static_cast<uint32_t>(m_swap_chain_images.size());

        VkDescriptorPoolCreateInfo pool_info_ = {};
        pool_info_.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info_.poolSizeCount = static_cast<uint32_t>(pool_sizes_.size());
        pool_info_.pPoolSizes = pool_sizes_.data();
        pool_info_.maxSets = static_cast<uint32_t>(m_swap_chain_images.size());

        pool_info_.maxSets = static_cast<uint32_t>(m_swap_chain_images.size());

        if (vkCreateDescriptorPool(m_device, &pool_info_, nullptr, &m_descriptorpool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");
    }

    void create_descriptorsets()
    {
        std::vector<VkDescriptorSetLayout> layouts_(m_swap_chain_images.size(), m_descriptorset_layout);
        VkDescriptorSetAllocateInfo alloc_info_ = {};
        alloc_info_.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info_.descriptorPool = m_descriptorpool;
        alloc_info_.descriptorSetCount = static_cast<uint32_t>(m_swap_chain_images.size());
        alloc_info_.pSetLayouts = layouts_.data();

        m_descriptorsets.resize(m_swap_chain_images.size());

        if (vkAllocateDescriptorSets(m_device, &alloc_info_, m_descriptorsets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets!");

        for (size_t i = 0; i < m_swap_chain_images.size(); ++i)
        {
            VkDescriptorBufferInfo buffer_info_ = {};
            buffer_info_.buffer = m_uniform_buffers[i];
            buffer_info_.offset = 0;
            buffer_info_.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo image_info_ = {};
            image_info_.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info_.imageView = m_texture_image_view;
            image_info_.sampler = m_texture_sampler;

            std::array<VkWriteDescriptorSet, 2> descriptor_writes_ = {};

            descriptor_writes_[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes_[0].dstSet = m_descriptorsets[i];
            descriptor_writes_[0].dstBinding = 0;
            descriptor_writes_[0].dstArrayElement = 0;
            descriptor_writes_[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes_[0].descriptorCount = 1;
            descriptor_writes_[0].pBufferInfo = &buffer_info_;
            descriptor_writes_[0].pImageInfo = nullptr;
            descriptor_writes_[0].pTexelBufferView = nullptr;

            descriptor_writes_[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes_[1].dstSet = m_descriptorsets[i];
            descriptor_writes_[1].dstBinding = 1;
            descriptor_writes_[1].dstArrayElement = 0;
            descriptor_writes_[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes_[1].descriptorCount = 1;
            descriptor_writes_[1].pImageInfo = &image_info_;

            vkUpdateDescriptorSets(m_device,
                                   static_cast<uint32_t>(descriptor_writes_.size()),
                                   descriptor_writes_.data(),
                                   0,
                                   nullptr);
        }
    }

    void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo image_info_ = {};
        image_info_.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info_.imageType = VK_IMAGE_TYPE_2D;
        image_info_.extent.width = width;
        image_info_.extent.height = height;
        image_info_.extent.depth = 1;
        image_info_.mipLevels = 1;
        image_info_.arrayLayers = 1;
        image_info_.format = format;
        image_info_.tiling = tiling;
        image_info_.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info_.usage = usage;
        image_info_.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info_.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info_.flags = 0;

        if (vkCreateImage(m_device, &image_info_, nullptr, &image) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image!");

        VkMemoryRequirements mem_requirements_;
        vkGetImageMemoryRequirements(m_device, image, &mem_requirements_);

        VkMemoryAllocateInfo alloc_info_ = {};
        alloc_info_.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info_.allocationSize = mem_requirements_.size;
        alloc_info_.memoryTypeIndex = find_memory_type(mem_requirements_.memoryTypeBits, properties);

        if (vkAllocateMemory(m_device, &alloc_info_, nullptr, &imageMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate image memory!");

        vkBindImageMemory(m_device, image, imageMemory, 0);
    }

    void create_textureimage()
    {
        int tex_width_;
        int tex_height_;
        int tex_channels_;

        stbi_uc* pixels_ = stbi_load("textures/texture.jpg", &tex_width_, &tex_height_, &tex_channels_, STBI_rgb_alpha);
        VkDeviceSize image_size_ = tex_width_ * tex_height_ * 4;

        if (!pixels_)
            throw std::runtime_error("Failed to load texture image!");

        VkBuffer staging_buffer_;
        VkDeviceMemory staging_buffer_memory_;

        create_buffer(image_size_,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      staging_buffer_,
                      staging_buffer_memory_);

        void* data_;

        vkMapMemory(m_device, staging_buffer_memory_, 0, image_size_, 0, &data_);
        memcpy(data_, pixels_, static_cast<size_t>(image_size_));
        vkUnmapMemory(m_device, staging_buffer_memory_);

        stbi_image_free(pixels_);

        create_image(tex_width_,
                     tex_height_,
                     VK_FORMAT_R8G8B8A8_UNORM,
                     VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_texture_image,
                     m_texture_image_memory);

        transition_image_layout(m_texture_image,
                                VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_buffer_to_image(staging_buffer_,
                             m_texture_image,
                             static_cast<uint32_t>(tex_width_),
                             static_cast<uint32_t>(tex_height_));
        transition_image_layout(m_texture_image,
                                VK_FORMAT_R8G8B8A8_UNORM,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(m_device, staging_buffer_, nullptr);
        vkFreeMemory(m_device, staging_buffer_memory_, nullptr);
    }

    VkCommandBuffer begin_single_time_commands()
    {
        VkCommandBufferAllocateInfo alloc_info_ {};
        alloc_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info_.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info_.commandPool = m_commandpool;
        alloc_info_.commandBufferCount = 1;

        VkCommandBuffer command_buffer_;
        vkAllocateCommandBuffers(m_device, &alloc_info_, &command_buffer_);

        VkCommandBufferBeginInfo begin_info_ = {};
        begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer_, &begin_info_);

        return command_buffer_;
    }

    void end_single_time_commands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submit_info_ = {};
        submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info_.commandBufferCount = 1;
        submit_info_.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_graphics_queue, 1, &submit_info_, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphics_queue);

        vkFreeCommandBuffers(m_device, m_commandpool, 1, &commandBuffer);
    }

    void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBuffer command_buffer_ = begin_single_time_commands();

        VkImageMemoryBarrier barrier_ = {};
        barrier_.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_.oldLayout = oldLayout;
        barrier_.newLayout = newLayout;

        barrier_.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier_.image = image;
        barrier_.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier_.subresourceRange.baseMipLevel = 0;
        barrier_.subresourceRange.levelCount = 1;
        barrier_.subresourceRange.baseArrayLayer = 0;
        barrier_.subresourceRange.layerCount = 1;


        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier_.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (has_stencil_component(format))
                barrier_.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
            barrier_.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        barrier_.srcAccessMask = 0; // TODO
        barrier_.dstAccessMask = 0; // TODO

        VkPipelineStageFlags source_stage_;
        VkPipelineStageFlags destination_stage_;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier_.srcAccessMask = 0;
            barrier_.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            source_stage_ = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage_ = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier_.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier_.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            source_stage_ = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage_ = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier_.srcAccessMask = 0;
            barrier_.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            source_stage_ = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage_ = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
          throw std::invalid_argument("Unsupported layout transition!");

        vkCmdPipelineBarrier(command_buffer_,
                             source_stage_,
                             destination_stage_,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier_);

        end_single_time_commands(command_buffer_);
    }

    void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
    {
      VkCommandBuffer command_buffer_ = begin_single_time_commands();

      VkBufferImageCopy region_ = {};
      region_.bufferOffset = 0;
      region_.bufferRowLength = 0;
      region_.bufferImageHeight = 0;
      region_.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region_.imageSubresource.mipLevel = 0;
      region_.imageSubresource.baseArrayLayer = 0;
      region_.imageSubresource.layerCount = 1;
      region_.imageOffset = {0, 0, 0};
      region_.imageExtent = { width, height, 1 };

      vkCmdCopyBufferToImage(command_buffer_,
                             buffer,
                             image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1,
                             &region_);

      end_single_time_commands(command_buffer_);
    }

    void create_textureimageview()
    {
        m_texture_image_view = create_image_view(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
    {
        VkImageView image_view_;

        VkImageViewCreateInfo create_info_ = {};
        create_info_.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info_.image = image;
        create_info_.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info_.format = format;
        create_info_.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info_.subresourceRange.aspectMask = aspectFlags;
        create_info_.subresourceRange.baseMipLevel = 0;
        create_info_.subresourceRange.levelCount = 1;
        create_info_.subresourceRange.baseArrayLayer = 0;
        create_info_.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &create_info_, nullptr, &image_view_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create texture image view!");

        return image_view_;
    }

    void create_texturesampler()
    {
        VkSamplerCreateInfo sampler_info_ = {};
        sampler_info_.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info_.magFilter = VK_FILTER_LINEAR;
        sampler_info_.minFilter = VK_FILTER_LINEAR;
        sampler_info_.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info_.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info_.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info_.anisotropyEnable = VK_TRUE;
        sampler_info_.maxAnisotropy = 16;
        sampler_info_.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info_.unnormalizedCoordinates = VK_FALSE;
        sampler_info_.compareEnable = VK_FALSE;
        sampler_info_.compareOp = VK_COMPARE_OP_ALWAYS;
        sampler_info_.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info_.mipLodBias = 0.0f;
        sampler_info_.minLod = 0.0f;
        sampler_info_.maxLod = 0.0f;

        if (vkCreateSampler(m_device, &sampler_info_, nullptr, &m_texture_sampler) != VK_SUCCESS)
            throw std::runtime_error("Failed to create texture sampler!");
    }

    VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props_;
            vkGetPhysicalDeviceFormatProperties(m_physical_device, format, &props_);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props_.linearTilingFeatures & features) == features)
                return format;
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props_.optimalTilingFeatures & features) == features)
                return format;
        }

        throw std::runtime_error("Failed to find supported format!");
    }

    bool has_stencil_component(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat find_depth_format()
    {
        return find_supported_format({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                     VK_IMAGE_TILING_OPTIMAL,
                                     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void create_depth_resources()
    {
        VkFormat depth_format_ = find_depth_format();

        create_image(m_swap_chain_extent.width,
                     m_swap_chain_extent.height,
                     depth_format_,
                     VK_IMAGE_TILING_OPTIMAL,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_depth_image,
                     m_depth_image_memory);

        m_depth_image_view = create_image_view(m_depth_image, depth_format_, VK_IMAGE_ASPECT_DEPTH_BIT);

        transition_image_layout(m_depth_image, depth_format_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


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
    VkDescriptorSetLayout m_descriptorset_layout;
    VkPipelineLayout m_pipeline_layout;
    VkRenderPass m_render_pass;
    VkPipeline m_graphics_pipeline;
    std::vector<VkFramebuffer> m_swap_chain_framebuffers;
    VkCommandPool m_commandpool;
    std::vector<VkCommandBuffer> m_commandbuffers;

    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_inflight_fences;
    size_t m_current_frame;
    bool m_framebuffer_resized;

    VkBuffer m_vertexbuffer;
    VkDeviceMemory m_vertexbuffer_memory;

    VkBuffer m_indexbuffer;
    VkDeviceMemory m_indexbuffer_memory;

    std::vector<VkBuffer> m_uniform_buffers;
    std::vector<VkDeviceMemory> m_uniformbuffers_memory;
    VkDescriptorPool m_descriptorpool;
    std::vector<VkDescriptorSet> m_descriptorsets;

    VkImage m_texture_image;
    VkDeviceMemory m_texture_image_memory;
    VkImageView m_texture_image_view;
    VkSampler m_texture_sampler;

    VkImage m_depth_image;
    VkDeviceMemory m_depth_image_memory;
    VkImageView m_depth_image_view;

    std::shared_ptr<GLFWwindow*> m_window;
};

#endif