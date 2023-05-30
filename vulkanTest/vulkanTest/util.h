#pragma once

#include "command.h"
#include <stdexcept>
#include <vulkan/vulkan.h>

// Memory
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// Image
void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                 VkDeviceMemory &imageMemory);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
void transitionImageLayout(VkDevice device, VkImage image, VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout, Command command);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, Command command);
