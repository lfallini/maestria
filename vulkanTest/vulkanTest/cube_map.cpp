#include "cube_map.h"

void CubeMap::createTextureImage(Command command) {

  stbi_load(faces[0].c_str(), &texWidth, &texHeight, nullptr, STBI_rgb_alpha);

  VkDeviceSize imageSize     = 6 * texWidth * texHeight * 4;
  Buffer       stagingBuffer = Buffer(device, physicalDevice);

  stagingBuffer.init(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);

  stbi_uc     *data;
  VkDeviceSize layerSize = imageSize / 6;
  vkMapMemory(device, stagingBuffer.memory, 0, imageSize, 0, (void **) &data);

  int i = 0;
  for (int i = 0; i < faces.size(); i++) {
    stbi_uc *pixels = stbi_load(faces[i].c_str(), &texWidth, &texHeight, nullptr, STBI_rgb_alpha);

    if (!pixels) {
      throw std::runtime_error("failed to load texture image!");
    }

    memcpy(data + layerSize * i, pixels, static_cast<size_t>(layerSize));

    stbi_image_free(pixels);
  }

  createImage(device, physicalDevice, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image,
              memory, getArrayLayers(), getFlags());

  transitionImageLayout(device, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command, getLayerCount());

  command.beginSingleTimeCommands();
  std::vector<VkBufferImageCopy> bufferCopyRegions;

  for (uint32_t i = 0; i < faces.size(); i++) {


    VkBufferImageCopy region{};
    region.bufferOffset                    = layerSize * i;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = i;
    region.imageSubresource.layerCount     = 1;
    region.imageExtent.width               = texWidth;
    region.imageExtent.height              = texHeight;
    region.imageExtent.depth               = 1;

    bufferCopyRegions.emplace_back(region);
    
  }
  
  vkCmdCopyBufferToImage(command.commandBuffer, stagingBuffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bufferCopyRegions.size(),
                         bufferCopyRegions.data());

  command.endSingleTimeCommands();


  transitionImageLayout(device, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command, getLayerCount());

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.memory, nullptr);
}
