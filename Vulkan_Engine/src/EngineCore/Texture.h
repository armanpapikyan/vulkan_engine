#pragma once
#include "pch.h"
#include "Common.h"

namespace Presentation
{
	class Device;
}

namespace boost {
	namespace serialization {
		class access;
	}
}

struct ITextureContainer
{
	virtual uint32_t getByteSize() const = 0;
	virtual void copyToMappedBuffer(void* destination, size_t offset = 0) const = 0;

protected:
	static void copy(void* destination, size_t offset, const void* source, size_t byteSize);
};

struct MipDesc
{
	uint32_t width, height;
	uint32_t imageByteSize;

	MipDesc(uint32_t w, uint32_t h, uint32_t byteSize);
};

struct LoadedTexture : ITextureContainer
{
	LoadedTexture(LoadedTexture&& texture) = default;
	LoadedTexture(const LoadedTexture& other) = default;
	LoadedTexture(unsigned char* const data, uint32_t byteSize, uint32_t width, uint32_t height);

	uint32_t getByteSize() const override;
	MipDesc getDimensions() const;
	void copyToMappedBuffer(void* destination, size_t offset = 0) const override;

	void release() noexcept;

private:
	std::vector<unsigned char> pixels;
	uint32_t imageByteSize;

	uint32_t width, height;
};

struct Texture
{
	Texture();
	Texture(std::vector<LoadedTexture>&& loadedTextures, VkFormat f, uint32_t w, uint32_t h, uint32_t ch);

	void Init(std::vector<LoadedTexture>&& loadedTextures, VkFormat f, uint32_t w, uint32_t h, uint32_t ch);

	bool hasPixelData() const;
	const std::vector<LoadedTexture>& getMipChain() const;
	void releasePixelData() noexcept;

	std::vector<LoadedTexture> textureMipChain;
	VkFormat format;
	uint32_t width;
	uint32_t height;
	uint32_t channels;

	~Texture();

	constexpr static float c_anisotropySamples = 4.0f;

	static bool tryLoadSupportedFormat(Texture& texture, const std::string& path);

	static void copyBufferToImage(const Presentation::Device* presentationDevice, VkBuffer buffer, VkImage image, const std::vector<MipDesc>& dimensions);
	static void transitionImageLayout(const Presentation::Device* presentationDevice, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipCount);
	static void transitionImageLayout(const VkCommandBuffer cmd, const VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipCount = 1u, VkImageAspectFlagBits subResImageAspect = VK_IMAGE_ASPECT_COLOR_BIT);

private:
	static bool stbiLoad(Texture& texture, const std::string& path);
	static bool ddsLoad(Texture& texture, const std::string& path);
};