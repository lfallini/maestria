#include "rt_vulkan_app.h"

void RtVulkanApp::initPointerFunctions() {
  pfnCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
      vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
  pfnGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
      vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
  pfnCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
      vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
  pfnGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
      vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
  pfnCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
      vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
  pfnGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
      vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
  pfnCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
  pfnGetBufferDeviceAddressKHR =
      reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
}

void RtVulkanApp::modelToVkGeometryKHR() {

  // BLAS builder requires raw device addresses
  VkDeviceAddress indexAddress  = getBufferDeviceAddress(device, indexBuffer.buffer);
  VkDeviceAddress vertexAddress = getBufferDeviceAddress(device, vertexBuffer.buffer);

  uint32_t maxPrimitiveCount = static_cast<uint32_t>(indices.size()) / 3;

  VkAccelerationStructureGeometryTrianglesDataKHR triangles{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
  triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
  triangles.vertexData.deviceAddress = vertexAddress;
  triangles.vertexStride             = sizeof(Vertex::pos);
  // Describe index data (32-bit unsigned int)
  triangles.indexType               = VK_INDEX_TYPE_UINT32;
  triangles.indexData.deviceAddress = indexAddress;

  // Indicate identity transform by setting transformData to null device pointer
  // triangles.transformData = {};
  triangles.maxVertex = vertices.size();

  // Identify the above data as containing opaque triangles
  VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  asGeom.geometryType       = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  asGeom.flags              = VK_GEOMETRY_OPAQUE_BIT_KHR;
  asGeom.geometry.triangles = triangles;

  // The entire array will be used to build the BLAS
  VkAccelerationStructureBuildRangeInfoKHR offset;
  offset.firstVertex     = 0;
  offset.primitiveCount  = maxPrimitiveCount;
  offset.primitiveOffset = 0;
  offset.transformOffset = 0;
}

void RtVulkanApp::createStorageImage() {
  // Set up a storage image that the ray generation shader will be writing to
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);

  createImage(device, physicalDevice, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              storageImage.image, storageImage.memory);

  storageImage.width  = width;
  storageImage.height = height;
  storageImage.view = createImageView(device, storageImage.image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
  transitionImageLayout(storageImage.image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_GENERAL);
}

void RtVulkanApp::createBottomLevelAS() {
  // Setup vertices and indices for a single triangle

  createTransformMatrixBuffer();

  VkDeviceAddress indexBufferAddress           = getBufferDeviceAddress(device, indexBuffer.buffer);
  VkDeviceAddress vertexBufferAddress          = getBufferDeviceAddress(device, vertexBuffer.buffer);
  VkDeviceAddress transformMatrixBufferAddress = getBufferDeviceAddress(device, transformMatrixBuffer.buffer);

  // The bottom level acceleration structure contains one set of triangles as
  // the input geometry
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  accelerationStructureGeometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  accelerationStructureGeometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
  accelerationStructureGeometry.geometry.triangles.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  accelerationStructureGeometry.geometry.triangles.vertexFormat                = VK_FORMAT_R32G32B32_SFLOAT;
  accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress    = vertexBufferAddress;
  accelerationStructureGeometry.geometry.triangles.maxVertex                   = vertices.size();
  accelerationStructureGeometry.geometry.triangles.vertexStride                = sizeof(Vertex);
  accelerationStructureGeometry.geometry.triangles.indexType                   = VK_INDEX_TYPE_UINT32;
  accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress     = indexBufferAddress;
  accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = transformMatrixBufferAddress;

  // Get the size requirements for buffers involved in the acceleration
  // structure build process
  VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
  accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  accelerationStructureBuildGeometryInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  accelerationStructureBuildGeometryInfo.geometryCount = 1;
  accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

  // number of triangles
  uint32_t primitiveCount = indices.size() / 3;

  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
  accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  pfnGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                           &accelerationStructureBuildGeometryInfo, &primitiveCount,
                                           &accelerationStructureBuildSizesInfo);

  // Create a buffer to hold the acceleration structure
  bottomLevelAccelerationStructure.buffer = Buffer(device, physicalDevice);
  bottomLevelAccelerationStructure.buffer.init(accelerationStructureBuildSizesInfo.accelerationStructureSize,
                                               VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, 0, 0);

  // Create the acceleration structure
  VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureCreateInfo.buffer = bottomLevelAccelerationStructure.buffer.buffer;
  accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
  accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

  if (pfnCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr,
                                        &bottomLevelAccelerationStructure.handle) != VK_SUCCESS) {
    throw std::runtime_error("Error creating acceleration structure");
  }

  // The actual build process starts here

  // Create a scratch buffer as a temporary storage for the acceleration
  // structure build
  Buffer scratchBuffer = Buffer(device, physicalDevice);
  scratchBuffer.init(accelerationStructureBuildSizesInfo.buildScratchSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
  accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  accelerationBuildGeometryInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  accelerationBuildGeometryInfo.mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  accelerationBuildGeometryInfo.dstAccelerationStructure  = bottomLevelAccelerationStructure.handle;
  accelerationBuildGeometryInfo.geometryCount             = 1;
  accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
  accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(device, scratchBuffer.buffer);

  VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
  accelerationStructureBuildRangeInfo.primitiveCount                                           = primitiveCount;
  accelerationStructureBuildRangeInfo.primitiveOffset                                          = 0;
  accelerationStructureBuildRangeInfo.firstVertex                                              = 0;
  accelerationStructureBuildRangeInfo.transformOffset                                          = 0;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
      &accelerationStructureBuildRangeInfo};

  // Build the acceleration structure on the device via a one-time command
  // buffer submission Some implementations may support acceleration structure
  // building on the host
  // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
  // but we prefer device builds
  Command         cmd           = Command(device, commandPool, graphicsQueue);
  VkCommandBuffer commandBuffer = cmd.beginSingleTimeCommands();

  pfnCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo,
                                       accelerationBuildStructureRangeInfos.data());

  cmd.endSingleTimeCommands();

  vkDestroyBuffer(device, scratchBuffer.buffer, nullptr);
  vkFreeMemory(device, scratchBuffer.memory, nullptr);

  // Get the bottom acceleration structure's handle, which will be used during
  // the top level acceleration build
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAccelerationStructure.handle;
  bottomLevelAccelerationStructure.deviceAddress =
      pfnGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
}

void RtVulkanApp::updateInstancesBuffer(VkAccelerationStructureInstanceKHR accelerationStructureInstance,
                                        VkBuffer                           instancesBuffer) {
  VkDeviceSize bufferSize = sizeof(VkAccelerationStructureInstanceKHR);

  Buffer stagingBuffer = Buffer(device, physicalDevice);
  stagingBuffer.init(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);

  void *data;
  vkMapMemory(device, stagingBuffer.memory, 0, bufferSize, 0, &data);
  memcpy(data, &accelerationStructureInstance, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBuffer.memory);

  copyBuffer(stagingBuffer.buffer, instancesBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.memory, nullptr);
}
void RtVulkanApp::createTopLevelAS() {
  VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
  accelerationStructureInstance.transform                              = transformMatrix;
  accelerationStructureInstance.instanceCustomIndex                    = 0;
  accelerationStructureInstance.mask                                   = 0xFF;
  accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
  accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  accelerationStructureInstance.accelerationStructureReference = bottomLevelAccelerationStructure.deviceAddress;

  Buffer instancesBuffer = Buffer(device, physicalDevice);
  instancesBuffer.init(sizeof(VkAccelerationStructureInstanceKHR),
                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       0, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  updateInstancesBuffer(accelerationStructureInstance, instancesBuffer.buffer);

  VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
  instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(device, instancesBuffer.buffer);

  // The top level acceleration structure contains (bottom level) instance as
  // the input geometry
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  accelerationStructureGeometry.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  accelerationStructureGeometry.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
  accelerationStructureGeometry.geometry.instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
  accelerationStructureGeometry.geometry.instances.data            = instanceDataDeviceAddress;

  // Get the size requirements for buffers involved in the acceleration
  // structure build process
  VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
  accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  accelerationStructureBuildGeometryInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  accelerationStructureBuildGeometryInfo.geometryCount = 1;
  accelerationStructureBuildGeometryInfo.pGeometries   = &accelerationStructureGeometry;

  const uint32_t primitive_count = 1;

  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
  accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  pfnGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                           &accelerationStructureBuildGeometryInfo, &primitive_count,
                                           &accelerationStructureBuildSizesInfo);

  // Create a buffer to hold the acceleration structure
  topLevelAccelerationStructure.buffer = Buffer(device, physicalDevice);
  topLevelAccelerationStructure.buffer.init(accelerationStructureBuildSizesInfo.accelerationStructureSize,
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, 0,
                                            VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  // Create the acceleration structure
  VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  accelerationStructureCreateInfo.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureCreateInfo.buffer = topLevelAccelerationStructure.buffer.buffer;
  accelerationStructureCreateInfo.size   = accelerationStructureBuildSizesInfo.accelerationStructureSize;
  accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  pfnCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr,
                                    &topLevelAccelerationStructure.handle);

  // The actual build process starts here

  // Create a scratch buffer as a temporary storage for the acceleration
  // structure build
  Buffer scratchBuffer = Buffer(device, physicalDevice);
  scratchBuffer.init(accelerationStructureBuildSizesInfo.buildScratchSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
  accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  accelerationBuildGeometryInfo.type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  accelerationBuildGeometryInfo.mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  accelerationBuildGeometryInfo.dstAccelerationStructure  = topLevelAccelerationStructure.handle;
  accelerationBuildGeometryInfo.geometryCount             = 1;
  accelerationBuildGeometryInfo.pGeometries               = &accelerationStructureGeometry;
  accelerationBuildGeometryInfo.scratchData.deviceAddress = getBufferDeviceAddress(device, scratchBuffer.buffer);

  VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
  accelerationStructureBuildRangeInfo.primitiveCount                                           = 1;
  accelerationStructureBuildRangeInfo.primitiveOffset                                          = 0;
  accelerationStructureBuildRangeInfo.firstVertex                                              = 0;
  accelerationStructureBuildRangeInfo.transformOffset                                          = 0;
  std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
      &accelerationStructureBuildRangeInfo};

  // Build the acceleration structure on the device via a one-time command
  // buffer submission Some implementations may support acceleration structure
  // building on the host
  // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
  // but we prefer device builds
  Command         cmd           = Command(device, commandPool, graphicsQueue);
  VkCommandBuffer commandBuffer = cmd.beginSingleTimeCommands();

  pfnCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo,
                                       accelerationBuildStructureRangeInfos.data());

  cmd.endSingleTimeCommands();

  vkDestroyBuffer(device, scratchBuffer.buffer, nullptr);
  vkFreeMemory(device, scratchBuffer.memory, nullptr);

  // Get the top acceleration structure's handle, which will be used to setup
  // it's descriptor
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure = topLevelAccelerationStructure.handle;
  topLevelAccelerationStructure.deviceAddress =
      pfnGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);
}

void RtVulkanApp::createRayTracingPipeline() {
  // Slot for binding top level acceleration structures to the ray generation
  // shader
  VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
  accelerationStructureLayoutBinding.binding         = 0;
  accelerationStructureLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  accelerationStructureLayoutBinding.descriptorCount = 1;
  accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
  resultImageLayoutBinding.binding         = 1;
  resultImageLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  resultImageLayoutBinding.descriptorCount = 1;
  resultImageLayoutBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  VkDescriptorSetLayoutBinding uniformBufferBinding{};
  uniformBufferBinding.binding         = 2;
  uniformBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBufferBinding.descriptorCount = 1;
  uniformBufferBinding.stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

  // Scene description
  VkDescriptorSetLayoutBinding sceneBufferBinding{};
  sceneBufferBinding.binding         = 3;
  sceneBufferBinding.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  sceneBufferBinding.descriptorCount = 1;
  sceneBufferBinding.stageFlags      = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding            = 4;
  samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount    = 1;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_MISS_BIT_KHR;

  std::vector<VkDescriptorSetLayoutBinding> bindings = {accelerationStructureLayoutBinding, resultImageLayoutBinding,
                                                        uniformBufferBinding, sceneBufferBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

  // setup push constants
  VkPushConstantRange pushConstant;
  // this push constant range starts at the beginning
  pushConstant.offset = 0;
  // this push constant range takes up the size of a PushConstant struct
  pushConstant.size = sizeof(PushConstant);
  // this push constant range is accessible only in the ray gen shader
  pushConstant.stageFlags                         = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;

  if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  /*
          Setup ray tracing shader groups
          Each shader group points at the corresponding shader in the pipeline
  */
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

  // Ray generation group
  {
    shaderStages.push_back(loadShader("shaders/raygen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR raygenGroupCi{};
    raygenGroupCi.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygenGroupCi.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygenGroupCi.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
    raygenGroupCi.closestHitShader   = VK_SHADER_UNUSED_KHR;
    raygenGroupCi.anyHitShader       = VK_SHADER_UNUSED_KHR;
    raygenGroupCi.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(raygenGroupCi);
  }

  // Ray miss group
  {
    shaderStages.push_back(loadShader("shaders/miss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR missGroupCi{};
    missGroupCi.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missGroupCi.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missGroupCi.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
    missGroupCi.closestHitShader   = VK_SHADER_UNUSED_KHR;
    missGroupCi.anyHitShader       = VK_SHADER_UNUSED_KHR;
    missGroupCi.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(missGroupCi);
  }

  // Ray miss (shadow) group
  {
    shaderStages.push_back(loadShader("shaders/missShadow.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR miss_group_ci{VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    miss_group_ci.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    miss_group_ci.generalShader      = static_cast<uint32_t>(shaderStages.size()) - 1;
    miss_group_ci.closestHitShader   = VK_SHADER_UNUSED_KHR;
    miss_group_ci.anyHitShader       = VK_SHADER_UNUSED_KHR;
    miss_group_ci.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(miss_group_ci);
  }

  // Ray closest hit group
  {
    shaderStages.push_back(loadShader("shaders/closesthit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
    VkRayTracingShaderGroupCreateInfoKHR closesHitGroupCi{};
    closesHitGroupCi.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    closesHitGroupCi.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closesHitGroupCi.generalShader      = VK_SHADER_UNUSED_KHR;
    closesHitGroupCi.closestHitShader   = static_cast<uint32_t>(shaderStages.size()) - 1;
    closesHitGroupCi.anyHitShader       = VK_SHADER_UNUSED_KHR;
    closesHitGroupCi.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(closesHitGroupCi);
  }

  /*
          Create the ray tracing pipeline
  */
  VkRayTracingPipelineCreateInfoKHR raytracingPipelineCreateInfo{};
  raytracingPipelineCreateInfo.sType                        = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  raytracingPipelineCreateInfo.stageCount                   = static_cast<uint32_t>(shaderStages.size());
  raytracingPipelineCreateInfo.pStages                      = shaderStages.data();
  raytracingPipelineCreateInfo.groupCount                   = static_cast<uint32_t>(shaderGroups.size());
  raytracingPipelineCreateInfo.pGroups                      = shaderGroups.data();
  raytracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 8;
  raytracingPipelineCreateInfo.layout                       = pipelineLayout;

  if (pfnCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &raytracingPipelineCreateInfo, nullptr,
                                      &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Error creating Ray tracing pipeline");
  }
}

inline uint32_t alignedSize(uint32_t value, uint32_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }

void RtVulkanApp::createShaderBindingTable() {
  /*
          Create the Shader Binding Tables that connects the ray tracing
     pipelines' programs and the  top-level acceleration structure

          SBT Layout used in this sample:

                  /-------------\
                  | raygen      |
                  |-------------|
                  | miss        |
                  |-------------|
                  | miss shadow |
                  |-------------|
                  | hit         |
                  \-------------/
  */

  // Index position of the groups in the generated ray tracing pipeline
  // To be generic, this should be pass in parameters
  std::vector<uint32_t> rgenIndex{0};
  std::vector<uint32_t> missIndex{1, 2};
  std::vector<uint32_t> hitIndex{3};

  const uint32_t           handleSize        = rayTracingPipelineProperties.shaderGroupHandleSize;
  const uint32_t           handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize,
                                                           rayTracingPipelineProperties.shaderGroupHandleAlignment);
  const uint32_t           handleAlignment   = rayTracingPipelineProperties.shaderGroupHandleAlignment;
  const uint32_t           groupCount = static_cast<uint32_t>(rgenIndex.size() + missIndex.size() + hitIndex.size());
  const uint32_t           sbtSize    = groupCount * handleSizeAligned;
  const VkBufferUsageFlags sbtBufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  // Create binding table buffers for each shader type
  raygenShaderBindingTable = Buffer(device, physicalDevice);
  missShaderBindingTable   = Buffer(device, physicalDevice);
  hitShaderBindingTable    = Buffer(device, physicalDevice);

  raygenShaderBindingTable.init(handleSizeAligned * rgenIndex.size(), sbtBufferUsageFlags,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
  missShaderBindingTable.init(handleSizeAligned * missIndex.size(), sbtBufferUsageFlags,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
  hitShaderBindingTable.init(handleSizeAligned * hitIndex.size(), sbtBufferUsageFlags,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  // Copy the pipeline's shader handles into a host buffer
  std::vector<uint8_t> shaderHandleStorage(sbtSize);
  if (pfnGetRayTracingShaderGroupHandlesKHR(device, graphicsPipeline, 0, groupCount, sbtSize,
                                            shaderHandleStorage.data()) != VK_SUCCESS) {
    throw std::runtime_error("Error getting Ray tracing Shader Group Handles");
  }

  // Copy the shader handles from the host buffer to the binding tables

  void *data;
  vkMapMemory(device, raygenShaderBindingTable.memory, 0, handleSizeAligned * rgenIndex.size(), 0, &data);
  memcpy(data, shaderHandleStorage.data(), (size_t)handleSizeAligned);
  vkUnmapMemory(device, raygenShaderBindingTable.memory);

  vkMapMemory(device, missShaderBindingTable.memory, 0, 2 * handleSizeAligned, 0, &data);
  memcpy(data, shaderHandleStorage.data() + handleSizeAligned, (size_t)handleSizeAligned * 2);
  vkUnmapMemory(device, missShaderBindingTable.memory);

  vkMapMemory(device, hitShaderBindingTable.memory, 0, handleSizeAligned, 0, &data);
  memcpy(data, shaderHandleStorage.data() + (/*TODO: 3 = 1 (raygen table) + 2 (miss table) */ 3 * handleSizeAligned),
         (size_t)handleSizeAligned);
  vkUnmapMemory(device, hitShaderBindingTable.memory);
}

void RtVulkanApp::createDescriptorSets() {
  std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                                                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
                                                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
                                                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes    = poolSizes.data();
  descriptorPoolCreateInfo.maxSets       = 1;

  if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts        = &descriptorSetLayout;
  descriptorSetAllocateInfo.descriptorSetCount = 1;

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  // Setup the descriptor for binding our top level acceleration structure to
  // the ray tracing shaders
  VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures    = &topLevelAccelerationStructure.handle;

  VkWriteDescriptorSet accelerationStructureWrite{};
  accelerationStructureWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  accelerationStructureWrite.dstSet          = descriptorSets[0];
  accelerationStructureWrite.dstBinding      = 0;
  accelerationStructureWrite.descriptorCount = 1;
  accelerationStructureWrite.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  // The acceleration structure descriptor has to be chained via pNext
  accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;

  VkDescriptorImageInfo imageDescriptor{};
  imageDescriptor.imageView   = storageImage.view;
  imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

  VkDescriptorBufferInfo bufferDescriptor{};
  bufferDescriptor.buffer = uniformBuffers[0].buffer;
  bufferDescriptor.range  = sizeof(UniformBufferObject); // VK_WHOLE_SIZE;
  bufferDescriptor.offset = 0;

  VkDescriptorBufferInfo vertexBufferDescriptor{};
  vertexBufferDescriptor.buffer = vertexBuffer.buffer;
  vertexBufferDescriptor.range  = VK_WHOLE_SIZE;
  vertexBufferDescriptor.offset = 0;

  VkDescriptorBufferInfo indexBufferDescriptor{};
  indexBufferDescriptor.buffer = indexBuffer.buffer;
  indexBufferDescriptor.range  = VK_WHOLE_SIZE;
  indexBufferDescriptor.offset = 0;

  VkDescriptorBufferInfo sceneDescriptor{};
  sceneDescriptor.buffer = sceneDesc.buffer;
  sceneDescriptor.range  = sizeof(ObjBuffers);
  sceneDescriptor.offset = 0;

  // CubeMap
  VkDescriptorImageInfo cubeMapInfo{};
  cubeMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  cubeMapInfo.imageView   = skybox.imageView;
  cubeMapInfo.sampler     = skybox.sampler;

  VkWriteDescriptorSet resultImageWrite{};
  resultImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  resultImageWrite.dstSet          = descriptorSets[0];
  resultImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  resultImageWrite.dstBinding      = 1;
  resultImageWrite.pImageInfo      = &imageDescriptor;
  resultImageWrite.descriptorCount = 1;

  VkWriteDescriptorSet uniformBufferWrite{};
  uniformBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  uniformBufferWrite.dstSet          = descriptorSets[0];
  uniformBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBufferWrite.dstBinding      = 2;
  uniformBufferWrite.pBufferInfo     = &bufferDescriptor;
  uniformBufferWrite.descriptorCount = 1;

  VkWriteDescriptorSet sceneBufferWrite{};
  sceneBufferWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  sceneBufferWrite.dstSet          = descriptorSets[0];
  sceneBufferWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  sceneBufferWrite.dstBinding      = 3;
  sceneBufferWrite.pBufferInfo     = &sceneDescriptor;
  sceneBufferWrite.descriptorCount = 1;

  VkWriteDescriptorSet cubeMapBufferWrite{};
  cubeMapBufferWrite.sType         = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  cubeMapBufferWrite.dstSet        = descriptorSets[0];
  cubeMapBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  cubeMapBufferWrite.dstBinding     = 4;
  cubeMapBufferWrite.pImageInfo     = &cubeMapInfo;
  cubeMapBufferWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {accelerationStructureWrite, resultImageWrite,
                                                           uniformBufferWrite, sceneBufferWrite, cubeMapBufferWrite};
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                         VK_NULL_HANDLE);
}

void set_image_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
                      VkImageSubresourceRange subresource_range,
                      VkPipelineStageFlags    src_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                      VkPipelineStageFlags    dst_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
  // Create an image barrier object
  VkImageMemoryBarrier barrier{};
  barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.oldLayout           = old_layout;
  barrier.newLayout           = new_layout;
  barrier.image               = image;
  barrier.subresourceRange    = subresource_range;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (old_layout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    barrier.srcAccessMask = 0;
    break;

  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // Image is preinitialized
    // Only valid as initial layout for linear images, preserves memory contents
    // Make sure host writes have been finished
    barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (new_layout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image will be used as a transfer source
    // Make sure any reads from the image have been finished
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image will be used as a color attachment
    // Make sure any writes to the color buffer have been finished
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (barrier.srcAccessMask == 0) {
      barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(command_buffer, src_mask, dst_mask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void RtVulkanApp::buildCommandBuffers(uint32_t imageIndex) {
  if (appSettings.width != storageImage.width || appSettings.height != storageImage.height) {
    // If the view port size has changed, we need to recreate the storage image
    vkDestroyImageView(device, storageImage.view, nullptr);
    vkDestroyImage(device, storageImage.image, nullptr);
    vkFreeMemory(device, storageImage.memory, nullptr);
    createStorageImage();
    // The descriptor also needs to be updated to reference the new image
    VkDescriptorImageInfo imageDescriptor{};
    imageDescriptor.imageView   = storageImage.view;
    imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet resultImageWrite{};
    resultImageWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultImageWrite.dstSet          = descriptorSets[0];
    resultImageWrite.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageWrite.dstBinding      = 1;
    resultImageWrite.pImageInfo      = &imageDescriptor;
    resultImageWrite.descriptorCount = 1;

    vkUpdateDescriptorSets(device, 1, &resultImageWrite, 0, VK_NULL_HANDLE);
    buildCommandBuffers(imageIndex);
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo{};
  commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  for (int32_t i = 0; i < commandBuffers.size(); ++i) {
    vkResetCommandBuffer(commandBuffers[i], 0);
    VK_CHECK(vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo));

    /*
            Setup the strided device address regions pointing at the shader
       identifiers in the shader binding table
    */

    const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize,
                                                   rayTracingPipelineProperties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device, raygenShaderBindingTable.buffer);
    raygenShaderSbtEntry.stride        = handleSizeAligned;
    raygenShaderSbtEntry.size          = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device, missShaderBindingTable.buffer);
    missShaderSbtEntry.stride        = handleSizeAligned;
    missShaderSbtEntry.size          = handleSizeAligned * 2;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(device, hitShaderBindingTable.buffer);
    hitShaderSbtEntry.stride        = handleSizeAligned;
    hitShaderSbtEntry.size          = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    constants.time = clock();
    // upload the matrix to the GPU via push constants
    vkCmdPushConstants(commandBuffers[i], pipelineLayout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(PushConstant),
                       &constants);

    /*
            Dispatch the ray tracing commands
    */
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, graphicsPipeline);
    vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1,
                            &descriptorSets[0], 0, 0);

    pfnCmdTraceRaysKHR(commandBuffers[i], &raygenShaderSbtEntry, &missShaderSbtEntry, &hitShaderSbtEntry,
                       &callableShaderSbtEntry, appSettings.width, appSettings.height, 1);

    /*
            Copy ray tracing output to swap chain image
    */

    // Prepare current swap chain image as transfer destination
    set_image_layout(commandBuffers[i], swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    // Prepare ray tracing output image as transfer source
    set_image_layout(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_GENERAL,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset      = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset      = {0, 0, 0};
    copyRegion.extent         = {appSettings.width, appSettings.height, 1};

    vkCmdCopyImage(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // Transition swap chain image back for presentation
    set_image_layout(commandBuffers[i], swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, subresourceRange);

    // Transition ray tracing output image back to general layout
    set_image_layout(commandBuffers[i], storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_IMAGE_LAYOUT_GENERAL, subresourceRange);

    VK_CHECK(vkEndCommandBuffer(commandBuffers[i]));
  }
}

void RtVulkanApp::initVulkan() {

  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  initPointerFunctions();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createCommandPool();
  loadModel();
  createVertexBuffer();
  createIndexBuffer();
  createMaterialBuffer();
  createBufferReferences();

  skybox = loadCubeMap();
  /* Ray tracing setup */
  getPhysicalDeviceRaytracingProperties();
  createStorageImage();
  createBottomLevelAS();
  createTopLevelAS();
  createUniformBuffers();
  createRayTracingPipeline();
  createShaderBindingTable();
  /* End Ray tracing setup */

  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();
}

void RtVulkanApp::bindPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, graphicsPipeline);
}

void RtVulkanApp::drawFrame() {

  if (vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX) == VK_TIMEOUT) {
    throw std::runtime_error("timeout!");
  }

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
                                          VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  // Only reset the fence if we are submitting work
  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  buildCommandBuffers(imageIndex);
  updateUniformBuffer(currentFrame);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore          waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount         = 1;
  submitInfo.pWaitSemaphores            = waitSemaphores;
  submitInfo.pWaitDstStageMask          = waitStages;
  submitInfo.commandBufferCount         = 1;
  submitInfo.pCommandBuffers            = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[]  = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapChains;
  presentInfo.pImageIndices   = &imageIndex;
  presentInfo.pResults        = nullptr;

  // Recreate the swap chain if needed
  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }
  // End recreate swap chain

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  constants.frameNumber++;
}

void RtVulkanApp::createTransformMatrixBuffer() {

  VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  VkDeviceSize bufferSize = sizeof(transformMatrix);

  Buffer stagingBuffer = Buffer(device, physicalDevice);
  stagingBuffer.init(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  void *data;
  vkMapMemory(device, stagingBuffer.memory, 0, bufferSize, 0, &data);
  memcpy(data, transformMatrix.matrix, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBuffer.memory);

  // Note that the buffer usage flags for buffers consumed by the bottom level
  // acceleration structure require special flags
  transformMatrixBuffer = Buffer(device, physicalDevice);
  transformMatrixBuffer.init(bufferSize,
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             0, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  copyBuffer(stagingBuffer.buffer, transformMatrixBuffer.buffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.memory, nullptr);
}

VkPipelineShaderStageCreateInfo RtVulkanApp::loadShader(const std::string &file, VkShaderStageFlagBits stage) {
  /* Load compiled Shaders */

  auto shaderCode = readFile(file);

  VkShaderModule shaderModule = createShaderModule(shaderCode);

  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage                           = stage;
  shaderStage.module                          = shaderModule;
  shaderStage.pName                           = "main";
  // assert(shader_stage.module != VK_NULL_HANDLE);
  // shaderModules.push_back(shader_stage.module);
  return shaderStage;
}

void RtVulkanApp::getPhysicalDeviceRaytracingProperties() {
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

  // Requesting ray tracing properties
  VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  prop2.pNext = &rtProperties;
  vkGetPhysicalDeviceProperties2(physicalDevice, &prop2);

  rayTracingPipelineProperties = rtProperties;
}

void RtVulkanApp::createBufferReferences() {

  objBuffers.vertices        = getBufferDeviceAddress(device, vertexBuffer.buffer);
  objBuffers.indices         = getBufferDeviceAddress(device, indexBuffer.buffer);
  objBuffers.materials       = getBufferDeviceAddress(device, matColorBuffer.buffer);
  objBuffers.materialIndices = getBufferDeviceAddress(device, matIndexBuffer.buffer);

  VkDeviceSize bufferSize = sizeof(ObjBuffers);

  Buffer stagingBuffer = Buffer(device, physicalDevice);
  stagingBuffer.init(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0);

  void *data;
  vkMapMemory(device, stagingBuffer.memory, 0, bufferSize, 0, &data);
  memcpy(data, &objBuffers, (size_t)bufferSize);
  vkUnmapMemory(device, stagingBuffer.memory);

  sceneDesc = Buffer(device, physicalDevice);
  sceneDesc.init(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

  copyBuffer(stagingBuffer.buffer, sceneDesc.buffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer.buffer, nullptr);
  vkFreeMemory(device, stagingBuffer.memory, nullptr);
}