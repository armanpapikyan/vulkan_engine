#include "pch.h"
#include "DescriptorPoolManager.h"

DescriptorPoolManager::DescriptorPoolManager(VkDevice device) : m_device(device), m_poolCollection() { m_instance = this; }

DescriptorPoolManager* DescriptorPoolManager::getInstance() { return m_instance; }

bool DescriptorPoolManager::isInitialized() const { return true; }

VkDescriptorPool DescriptorPoolManager::createNewPool(uint32_t size)
{
	VkDescriptorPool pool{};
	if (!vkinit::Descriptor::createDescriptorPool(pool, m_device, size))
	{
		printf("Was not able to allocate a new descriptor pool!\n");
		return VK_NULL_HANDLE;
	}

	m_poolCollection.push_back(pool);
	return m_poolCollection.back();
}

void DescriptorPoolManager::release()
{
	for (auto& pool : m_poolCollection)
	{
		vkDestroyDescriptorPool(m_device, pool, nullptr);
	}
}
