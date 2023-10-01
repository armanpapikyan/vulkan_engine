#pragma once
#include "pch.h"

struct VertexAttributes
{
public:
	VertexAttributes() = delete;
	VertexAttributes(std::vector<VkBuffer>& vertexBuffers, std::vector<VmaAllocation>& vertexMemoryRanges, std::vector<VkDeviceSize>& vertexOffsets, uint32_t bindingOffset = 0, uint32_t bindingCount = 0);
	VertexAttributes(VkBuffer vertexBuffer, VmaAllocation vertexMemoryRange, VkDeviceSize vertexOffset, uint32_t bindingOffset = 0, uint32_t bindingCount = 0);

	void bind(VkCommandBuffer commandBuffer);

	void destroy(VmaAllocator allocator);

private:
	std::vector<VkBuffer> buffers;
	std::vector<VmaAllocation> memoryRanges;
	std::vector<VkDeviceSize> offsets;
	uint32_t firstBinding;
	uint32_t bindingCount;
};
