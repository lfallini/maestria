#pragma once

#include "command.h"
#include <stdexcept>
#include <vulkan/vulkan.h>

// Memory
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// Image
void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                 VkDeviceMemory &imageMemory, uint32_t arrayLayers = 1, VkImageCreateFlags flags = 0);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
                            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);
void transitionImageLayout(VkDevice device, VkImage image, VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout, Command command, uint32_t layerCount = 1);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, Command command);
