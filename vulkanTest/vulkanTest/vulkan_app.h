#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#undef max
#include <tiny_obj_loader.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <cstring>
#include <iomanip>

#include "buffer.h"
#include "camera.h"
#include "cube_map.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "texture_2D.h"
#include "util.h"
#include <chrono>

#define VK_CHECK(x)                                                                                                    \
  do {                                                                                                                 \
    VkResult err = x;                                                                                                  \
    if (err) {                                                                                                         \
      std::cout << "Detected Vulkan error: " << std::to_string(err) << std::endl;                                      \
      abort();                                                                                                         \
    };                                                                                                                 \
  } while (0);

// const std::string BASE_PATH    = "./models/cornell-box";
// const std::string MODEL_PATH   = "./models/cornell-box/Corn ellBox-Original.obj";

const std::string BASE_PATH  = "./models/sphere";
const std::string MODEL_PATH = "./models/sphere/sphere.obj";

const std::string TEXTURE_PATH = "./viking_room.png";

const int MAX_FRAMES_IN_FLIGHT = 1;

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,

    // Ray tracing extensions
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

    // Required by VK_KHR_acceleration_structure
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

    // Required for VK_KHR_ray_tracing_pipeline
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,

    // Required by VK_KHR_spirv_1_4
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

    VK_KHR_8BIT_STORAGE_EXTENSION_NAME, VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
    // required by GL_EXT_debug_printf
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME

};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, pos);

    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 1;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, normal);

    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 2;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, color);

    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 3;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(Vertex, texCoord);

    return attributeDescriptions;
  }

  bool operator==(const Vertex &other) const {
    return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
  }
};

struct Material {
  glm::vec3 diffuse{0.7f};
  glm::vec3 specular{0.7f};
  float     shininess{0.f};
};

struct ObjBuffers {
  VkDeviceAddress vertices;
  VkDeviceAddress indices;
  VkDeviceAddress materials;
  VkDeviceAddress materialIndices;
};

struct PushConstant {
  uint32_t frameNumber = 0;
  uint32_t maxDepth    = 4;
  uint32_t numSamples  = MAXINT - 1;
  uint32_t time;
};

namespace std {
template <> struct hash<Vertex> {
  size_t operator()(Vertex const &vertex) const {
    return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};

} // namespace std

struct UniformBufferObject {
  // alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view_inverse;
  alignas(16) glm::mat4 proj_inverse;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t            fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR        capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR>   presentModes;
};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct AppSettings {
  uint32_t width = 1440, height = 900;
};

class VulkanApp {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  };
  virtual ~VulkanApp(){};
  VulkanApp(){};

protected:
  AppSettings                  appSettings;
  GLFWwindow                  *window;
  VkInstance                   instance;
  VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
  VkDevice                     device;
  VkQueue                      graphicsQueue;
  VkQueue                      presentQueue;
  VkSurfaceKHR                 surface;
  VkSwapchainKHR               swapChain;
  std::vector<VkImage>         swapChainImages;
  VkFormat                     swapChainImageFormat;
  VkExtent2D                   swapChainExtent;
  std::vector<VkImageView>     swapChainImageViews;
  VkDescriptorSetLayout        descriptorSetLayout;
  VkDescriptorPool             descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  VkPipelineLayout             pipelineLayout;
  VkRenderPass                 renderPass;
  VkPipeline                   graphicsPipeline;

  // Vertex buffer
  std::vector<Vertex> vertices;
  Buffer              vertexBuffer;

  // Index buffer
  std::vector<uint32_t> indices;
  Buffer                indexBuffer;

  // Material buffer
  Buffer matColorBuffer;
  Buffer matIndexBuffer;

  std::vector<Material> materials;
  std::vector<int32_t>  materialsIndex;

  // TODO: convert to std::<vector>.
  ObjBuffers objBuffers;

  Texture2D exampleTexture;

  VkImage        depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView    depthImageView;

  std::vector<Buffer> uniformBuffers;

  std::vector<VkFramebuffer>   swapChainFramebuffers;
  VkCommandPool                commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore>     imageAvailableSemaphores;
  std::vector<VkSemaphore>     renderFinishedSemaphores;
  std::vector<VkFence>         inFlightFences;

  Camera camera;
  double xpos, ypos;

  uint32_t currentFrame       = 0;
  bool     framebufferResized = false;

  // ImgUi
  ImGui_ImplVulkanH_Window wd;

  void initWindow();

  static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app                = reinterpret_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
    app->appSettings.width  = width;
    app->appSettings.height = height;
  }

  void createInstance();
  void virtual initVulkan();
  void     loadModel();
  bool     hasStencilComponent(VkFormat format);
  VkFormat findDepthFormat();
  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
  void     createDepthResources();
  void     copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  void     transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
  void     createDescriptorSets();
  void     createDescriptorPool();
  void     updateUniformBuffer(uint32_t currentImage);
  void     createUniformBuffers();
  void     createDescriptorSetLayout();
  void     createIndexBuffer();
  void     copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void     createVertexBuffer();
  void     createMaterialBuffer();
  void     createBufferReferences();
  void     cleanupSwapChain();
  void     recreateSwapChain();
  void     createSyncObjects();
  void     createCommandBuffers();
  void     recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
  void     createCommandPool();
  void     createFramebuffers();
  void virtual createRenderPass();
  void           createGraphicsPipeline();
  VkShaderModule createShaderModule(const std::vector<char> &code);
  void           createImageViews();
  void           createSwapChain();
  void           createSurface();
  void           createLogicalDevice();
  void           pickPhysicalDevice();
  bool           isDeviceSuitable(VkPhysicalDevice device);
  bool           checkDeviceExtensionSupport(VkPhysicalDevice device);
  void virtual bindPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline);
  QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR      chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR        chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D              chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  void                    initImgUi();
  void                    mainLoop();
  void virtual drawFrame();
  void         cleanup();
  bool         checkValidationLayerSupport();
  CubeMap      loadCubeMap();
  PushConstant constants;

  /* Mouse pointer handling */
  void cursorPositionCallback(double newXpos, double newYpos) {

    auto &io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
      return;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      glm::vec3 rotation = glm::vec3(newYpos, newXpos, 0.0f);
      camera.set_rotation(rotation);

      if (newXpos != xpos || newYpos != ypos) {
        constants.frameNumber = 0;
      }
      xpos = newXpos;
      ypos = newYpos;
    }
  }

  static void cursorPositionCallbackWrapper(GLFWwindow *window, double newXpos, double newYpos) {
    auto handler = static_cast<VulkanApp *>(glfwGetWindowUserPointer(window));
    handler->cursorPositionCallback(newXpos, newYpos);
  }

  static void check_vk_result(VkResult err) {
    if (err == 0)
      return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
      abort();
  }
};
