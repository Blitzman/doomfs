#ifndef VERTEX_HPP_
#define VERTEX_HPP_

#include <array>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_NONE
#include <glm/glm.hpp>

struct Vertex
{
  glm::vec3 m_pos;
  glm::vec3 m_color;
  glm::vec2 m_texcoord;

  static VkVertexInputBindingDescription get_binding_description()
  {
    VkVertexInputBindingDescription binding_description_ = {};

    binding_description_.binding = 0;
    binding_description_.stride = sizeof(Vertex);
    binding_description_.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description_;
  }

  static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions()
  {
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions_ = {};

    attribute_descriptions_[0].binding = 0;
    attribute_descriptions_[0].location = 0;
    attribute_descriptions_[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions_[0].offset = offsetof(Vertex, m_pos);

    attribute_descriptions_[1].binding = 0;
    attribute_descriptions_[1].location = 1;
    attribute_descriptions_[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions_[1].offset = offsetof(Vertex, m_color);

    attribute_descriptions_[2].binding = 0;
    attribute_descriptions_[2].location = 2;
    attribute_descriptions_[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions_[2].offset = offsetof(Vertex, m_texcoord);

    return attribute_descriptions_;
  }
};

#endif