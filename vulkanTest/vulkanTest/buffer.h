#pragma once

#include <stdexcept>
#include <vulkan/vulkan.h>
#include "util.h"

class Buffer {
public:
  VkBuffer buffer;
  VkDeviceMemory memory;
  void *map;

  Buffer(){};
  Buffer(VkDevice, VkPhysicalDevice);

  void init(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
            VkMemoryAllocateFlags memoryFlags);

private:
  VkDevice device;
  VkPhysicalDevice physicalDevice;
};
