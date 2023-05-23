#pragma once
#include "VkTypes/InitializersUtility.h"
#include "VkTypes/VkMemoryAllocator.h"

class StagingBufferPool
{
public:
	struct StgBuffer
	{
		VkBuffer buffer;
		VmaAllocation allocation;

		uint32_t totalByteSize;
	};

	// Storing stats on the buffer performance.
	struct Stats
	{
		int freeCount;
		int claimCount;

		int reusedMatchingBuffer;
		int reusedBiggerBuffer;

		int destroyedBuffer;
		int createdBuffer;

		Stats();

		void print();
	};

	bool claimAStagingBuffer(StgBuffer& buffer, uint32_t byteSize);
	void freeBuffer(const StgBuffer& buffer);
	void releaseAllResources();

	const Stats& getStatistics() const { return m_stats; }

private:
	bool indirection_allocateBuffer(StgBuffer& buffer, uint32_t size);
	void indirection_destroyBuffer(StgBuffer& buffer, bool increaseCounter = true);

	bool createNewBuffer(StgBuffer& buffer, uint32_t size);
	void claimedFromFreePool_impl(size_t index);
	void returnToFreePool_impl(const StgBuffer& buffer);

	void claim(const StgBuffer& buffer);
	void free(const StgBuffer& buffer);

	// The actual buffers
	std::vector<StgBuffer> claimedPool;
	std::vector<StgBuffer> freePool;

	Stats m_stats{};
};
