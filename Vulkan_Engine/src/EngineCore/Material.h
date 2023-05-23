#pragma once
#include "pch.h"
#include "ShaderSource.h"
#include "VkTypes/VkMaterialVariant.h"
#include "FileManager/Path.h"
#include "FileManager/Directories.h"

struct TextureSource
{
	Path path;
	VkFormat format;
	bool generateTheMips;
	// Texture type
	// Sampler type
	// Other params

	TextureSource(std::string&& path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool generateMips = true);

	bool operator ==(const TextureSource& other) const;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar& path.value;
		ar& format;
		ar& generateTheMips;
	}

	std::string getTextureName(bool includeExtension) const;

private:
	friend class boost::serialization::access;
	friend class Material;
	TextureSource();
};

namespace std 
{
	template <>
	struct hash<TextureSource>
	{
		std::size_t operator()(const TextureSource& src) const
		{
			return hash<std::string>()(src.path.value);
		}
	};
}

namespace Presentation {
	class Device;
	class PresentationTarget;
}
namespace boost {
	namespace serialization {
		class access;
	}
	namespace archive {
		class binary_oarchive;
		class binary_iarchive;
	}
}
struct VkTexture2D;
struct VkMaterialVariant;
struct VkShader;

class Material
{
public:
	Material(uint32_t shaderIdentifier, const TextureSource& source);
	Material(uint32_t shaderIdentifier, TextureSource&& source);

	const TextureSource& getTextureSource() const;
	uint32_t getShaderIdentifier() const;
	size_t getHash() const;
	
	void serialize(boost::archive::binary_oarchive& ar, const unsigned int version); // WRITE
	void serialize(boost::archive::binary_iarchive& ar, const unsigned int version); // READ

	bool operator ==(const Material& other) const;

private:
	friend class boost::serialization::access;
	Material();

	uint32_t m_shaderIdentifier;
	TextureSource m_textureParameters;

	void calculateHash();
	size_t m_hash;
};

struct VkMaterial
{
	VkMaterial(const VkShader& shader, const VkTexture2D& texture, const VkPipeline pipeline, const VkPipelineLayout pipelineLayout, const VkDescriptorSetLayout descriptorSetLayout, std::array<VkDescriptorSet, SWAPCHAIN_IMAGE_COUNT>& descriptorSets);

	const VkMaterialVariant& getMaterialVariant() const;
	void release(VkDevice device);

	const VkShader* shader;
	const VkTexture2D* texture;

	VkMaterialVariant variant;
};