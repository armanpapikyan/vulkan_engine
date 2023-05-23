#include "pch.h"
#include "Common.h"
#include "VkShader.h"
#include "ShaderSource.h"

VkShader::VkShader(VkDevice device, const ShaderSource& source)
	: vertShader(VK_NULL_HANDLE), fragShader(VK_NULL_HANDLE)
{
	std::vector<char> sourcecode;
	if (source.getVertexSource(sourcecode))
	{
		if(!createShaderModule(vertShader, sourcecode, device))
			printf("Failed to compile the shader '%s'.\n", source.fragmentPath.c_str());
	}

	if (source.getFragmentSource(sourcecode))
	{
		if(!createShaderModule(fragShader, sourcecode, device))
			printf("Failed to compile the shader '%s'.\n", source.fragmentPath.c_str());
	}
}

bool VkShader::createShaderModule(VkShaderModule& module, const std::vector<char>& code, VkDevice device)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	return vkCreateShaderModule(device, &createInfo, nullptr, &module) == VK_SUCCESS;
}

void VkShader::release(VkDevice device)
{
	vkDestroyShaderModule(device, vertShader, nullptr);
	vkDestroyShaderModule(device, fragShader, nullptr);
}

void VkShader::ensureDefaultShader(VkDevice device)
{
	if (globalShaderList.size() == 0)
	{
		VkShader::createGlobalShader(device, ShaderSource::getDefaultShader());

		VkShader::createGlobalShader(device, ShaderSource::getDepthOnlyShader());

		VkShader::createGlobalShader(device, ShaderSource::getDebugQuadShader());
	}
}

//static bool findShader(std::string shaderName);

const VkShader* VkShader::findShader(uint32_t identifier) { return identifier < globalShaderList.size() ? globalShaderList[identifier].get() : nullptr; }

const VkShader* VkShader::createGlobalShader(VkDevice device, const ShaderSource& source)
{
	insertShaderToGlobalList
	(
		device, VkShader(device, source)
	);

	return globalShaderList.back().get();
}

void VkShader::releaseGlobalShaderList(VkDevice device)
{
	if (globalShaderList.size() == 0)
		return;

	for (auto& shader : globalShaderList)
	{
		shader->release(device);
	}

	globalShaderList.clear();
}

void VkShader::insertShaderToGlobalList(VkDevice device, VkShader shader)
{
	globalShaderList.push_back(MAKEUNQ<VkShader>(std::move(shader)));
}
