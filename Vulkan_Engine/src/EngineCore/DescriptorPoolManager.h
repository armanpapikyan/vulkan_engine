#pragma once
#include "pch.h"
#include "VkTypes/InitializersUtility.h"
#include "Interfaces/IRequireInitialization.h"

class DescriptorPoolManager : IRequireInitialization
{
public:
	DescriptorPoolManager(VkDevice device);
	static DescriptorPoolManager* getInstance();

	virtual bool isInitialized() const override;
	VkDescriptorPool createNewPool(uint32_t size);

	void release();

private:
	inline static DescriptorPoolManager* m_instance = nullptr;
	VkDevice m_device;
	std::vector<VkDescriptorPool> m_poolCollection;
};
