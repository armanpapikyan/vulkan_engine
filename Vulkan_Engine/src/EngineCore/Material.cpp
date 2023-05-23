#include "pch.h"
#include "VkTypes/VkMaterialVariant.h"

#include "Material.h"
#include "Presentation/Device.h"
#include "Presentation/PresentationTarget.h"
#include <EngineCore/Texture.h>
#include "VkTypes/VkTexture.h"
#include "VkTypes/VkShader.h"

VkMaterial::VkMaterial(const VkShader& shader, const VkTexture2D& texture, const VkPipeline pipeline, const VkPipelineLayout pipelineLayout, const VkDescriptorSetLayout descriptorSetLayout, std::array<VkDescriptorSet, SWAPCHAIN_IMAGE_COUNT>& descriptorSets)
	: shader(&shader), texture(&texture), variant(pipeline, pipelineLayout, descriptorSetLayout, descriptorSets) { }

const VkMaterialVariant& VkMaterial::getMaterialVariant() const { return variant; }

void VkMaterial::release(VkDevice device)
{
	shader = nullptr;
	texture = nullptr;

	variant.release(device);
}

Material::Material(uint32_t shaderIdentifier, const TextureSource& source) :
	m_shaderIdentifier(shaderIdentifier), m_textureParameters(source) {
	calculateHash();
}

Material::Material(uint32_t shaderIdentifier, TextureSource&& source) :
	m_shaderIdentifier(shaderIdentifier), m_textureParameters(source) {
	calculateHash();
}

Material::Material() : m_shaderIdentifier(), m_textureParameters() { calculateHash(); }

void Material::calculateHash() { m_hash = std::hash<std::string>()(m_textureParameters.path); }

const TextureSource& Material::getTextureSource() const { return m_textureParameters; }
uint32_t Material::getShaderIdentifier() const { return m_shaderIdentifier; }
size_t Material::getHash() const { return m_hash; }

bool Material::operator ==(const Material& other) const
{
	return m_shaderIdentifier == other.m_shaderIdentifier &&
		m_textureParameters == other.m_textureParameters;
}

// WRITE
void Material::serialize(boost::archive::binary_oarchive& ar, const unsigned int version)
{
	ar& m_shaderIdentifier;

	const std::string compressedAlternative = Directories::getWorkingDirectory().combine(m_textureParameters.getTextureName(false) + ".dds");
	if (std::filesystem::exists(compressedAlternative))
	{
		m_textureParameters.path.value = compressedAlternative;
		m_textureParameters.format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		m_textureParameters.generateTheMips = false;
	}

	// Always serializing relative path
	m_textureParameters.path.removeDirectory(Directories::getWorkingDirectory());
	// printf("path: %s\n", Directories::getWorkingDirectory().combine(m_textureParameters.path.value).value.c_str());
	assert(std::filesystem::exists(Directories::getWorkingDirectory().combine(m_textureParameters.path.value).value));

	ar& m_textureParameters.path.value;
	ar& m_textureParameters.format;
	ar& m_textureParameters.generateTheMips;

	ar& m_hash;
}

// READ
void Material::serialize(boost::archive::binary_iarchive& ar, const unsigned int version)
{
	ar& m_shaderIdentifier;

	// Deserializing always relative path to the working directory
	ar& m_textureParameters.path.value;
	m_textureParameters.path = Directories::getWorkingDirectory().combine(m_textureParameters.path);

	ar& m_textureParameters.format;
	ar& m_textureParameters.generateTheMips;

	ar& m_hash;
}

TextureSource::TextureSource() : path(), format(VK_FORMAT_R8G8B8A8_SRGB), generateTheMips(true) { }
TextureSource::TextureSource(std::string&& path, VkFormat format, bool generateMips) : path(std::move(path)), format(format), generateTheMips(generateMips) { }

bool TextureSource::operator ==(const TextureSource& other) const
{
	return format == other.format && generateTheMips == other.generateTheMips &&
		(path == other.path || path.getFileName(false) == other.path.getFileName(false));
}

std::string TextureSource::getTextureName(bool includeExtension) const
{
	return path.getFileName(includeExtension);
}
