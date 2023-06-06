#pragma once
#include "texture.h"
#include <vector>
class CubeMap : public Texture {

public:
  CubeMap(){};
  CubeMap(std::vector<std::string> faces, VkDevice device, VkPhysicalDevice physicalDevice)
      : Texture(path, device, physicalDevice) {
    this->faces = faces;
  };

  std::vector<std::string> faces;

  uint32_t           getArrayLayers() { return 6; }
  uint32_t           getLayerCount() { return 6; }
  VkImageViewType    getImageViewType() { return VK_IMAGE_VIEW_TYPE_CUBE; }
  VkImageCreateFlags getFlags() { return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; }

  void createTextureImage(Command);
};
