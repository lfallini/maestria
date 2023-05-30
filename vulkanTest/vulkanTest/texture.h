#include <vulkan/vulkan.h>
#include <stdexcept>
#include <stb_image.h>
#include "util.h"
#include "buffer.h"

#pragma once
class Texture {

public:
  Texture(){};
  Texture(std::string path, VkDevice, VkPhysicalDevice);

  std::string path;
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;
  VkSampler sampler;

  void createTextureImage(Command);
  void createTextureImageView();
  void createTextureSampler();

};
