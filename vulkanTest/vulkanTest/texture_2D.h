#pragma once
#include "texture.h"
class Texture2D : public Texture {
public:
  Texture2D(){};
  Texture2D(std::string path, VkDevice device, VkPhysicalDevice physicalDevice)
      : Texture(path, device, physicalDevice) {}

  uint32_t                getArrayLayers() { return 1; }
  uint32_t                getLayerCount() { return 1; }
  VkImageViewType         getImageViewType() { return VK_IMAGE_VIEW_TYPE_2D; }
  VkImageCreateFlags      getFlags() { return 0; }
};
