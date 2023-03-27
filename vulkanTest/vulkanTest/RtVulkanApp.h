#pragma once
#include "VulkanApp.h"

#define VK_CHECK(x)                                                                         \
	do																						\
	{																						\
		VkResult err = x;																	\
		if (err)																			\
		{																					\
			std::cout << "Detected Vulkan error: " << std::to_string(err) << std::endl;     \
			abort();																		\
		};																					\
	} while (0);																			\

// Holds data for a scratch buffer used as a temporary storage during acceleration structure builds
struct ScratchBuffer
{
	uint64_t       device_address;
	VkBuffer       handle;
	VkDeviceMemory memory;
};

struct AccelerationStructure {
	VkAccelerationStructureKHR         handle;
	uint64_t                           deviceAddress;
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct StorageImage
{
	VkDeviceMemory memory;
	VkImage        image;
	VkImageView    view;
	VkFormat       format;
	uint32_t       width;
	uint32_t       height;
};

class RtVulkanApp : public VulkanApp
{
public:
	RtVulkanApp() {};
	~RtVulkanApp() {};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties;
//private:
	AccelerationStructure bottomLevelAccelerationStructure;
	AccelerationStructure topLevelAccelerationStructure;

	VkBuffer transformMatrixBuffer;
	VkDeviceMemory transformMatrixMemory;

	//VkPipeline            pipelineRT;
	//VkPipelineLayout      pipelineLayoutRT;
	//VkDescriptorSet       descriptorSetRT;
	//VkDescriptorSetLayout descriptorSetLayoutRT;

	StorageImage storageImage;
	//Buffer ubo;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

	// Pointer functions 
	PFN_vkCreateAccelerationStructureKHR pfnCreateAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR pfnGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR pfnCmdBuildAccelerationStructuresKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR pfnGetAccelerationStructureBuildSizesKHR;
	PFN_vkCreateRayTracingPipelinesKHR pfnCreateRayTracingPipelinesKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR pfnGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCmdTraceRaysKHR pfnCmdTraceRaysKHR;
	PFN_vkGetBufferDeviceAddressKHR pfnGetBufferDeviceAddressKHR;

	// Binding table buffers
	Buffer raygenShaderBindingTable;
	Buffer missShaderBindingTable;
	Buffer hitShaderBindingTable;

	void initPointerFunctions();
	void getPhysicalDeviceRaytracingProperties();
	void modelToVkGeometryKHR();
	void createStorageImage();
	void createBottomLevelAS();
	void createTopLevelAS();
	void createRayTracingPipeline();
	void createShaderBindingTable();
	void createDescriptorSets();
	void buildCommandBuffers(uint32_t imageIndex);
	
	void initVulkan() override;
	void bindPipeline(VkCommandBuffer commandBuffer, VkPipeline pipeline) override;

	void drawFrame() override;

	inline VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
	{
		if (buffer == VK_NULL_HANDLE)
			return 0ULL;

		VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = buffer;
		return pfnGetBufferDeviceAddressKHR(device, &info);
	}

	void createTransformMatrixBuffer();
	VkPipelineShaderStageCreateInfo loadShader(const std::string& file, VkShaderStageFlagBits stage);
	
};

