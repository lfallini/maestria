#include "buffer.h"
#include "util.h"
#include <stb_image.h>
#include <stdexcept>
#include <vulkan/vulkan.h>

#pragma once
class Texture {

public:
  Texture(){};
  Texture(std::string path, VkDevice, VkPhysicalDevice);

  std::string      path;
  int              texWidth, texHeight;
  VkDevice         device;
  VkPhysicalDevice physicalDevice;

  VkImage        image;
  VkDeviceMemory memory;
  VkImageView    imageView;
  VkSampler      sampler;

  void createTextureImage(Command);
  void createTextureImageView();
  void createTextureSampler();

  virtual uint32_t           getArrayLayers()   = 0;
  virtual uint32_t           getLayerCount()    = 0;
  virtual VkImageViewType    getImageViewType() = 0;
  virtual VkImageCreateFlags getFlags()         = 0;
};
