#pragma once
#include "pch.h"
#include "Common.h"

struct ShaderSource;

struct VkShader
{
	VkShaderModule vertShader,
		fragShader;

	VkShader(VkDevice device, const ShaderSource& source);
	static bool createShaderModule(VkShaderModule& module, const std::vector<char>& code, VkDevice device);

	static void ensureDefaultShader(VkDevice device);

	static const VkShader* findShader(uint32_t identifier);
	static const VkShader* createGlobalShader(VkDevice device, const ShaderSource& source);

	void release(VkDevice device);
	static void releaseGlobalShaderList(VkDevice device);

private:
	inline static std::vector<std::unique_ptr<VkShader>> globalShaderList;

	static void insertShaderToGlobalList(VkDevice device, VkShader shader);
};