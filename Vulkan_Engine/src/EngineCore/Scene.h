#pragma once
#include "pch.h"
#include "Common.h"

struct Mesh;
struct VkMesh;
struct ProcessedMesh;
struct Transform;
struct Renderer;

class Material;
struct TextureSource;
struct VkTexture2D;
struct VkMaterial;
struct VkMeshRenderer;

namespace Loader { struct ModelLoaderOptions; }

namespace Presentation
{
	class Device;
	class PresentationTarget;
}

class Scene
{
public:
	Scene(const Presentation::Device* device, Presentation::PresentationTarget* target);
	~Scene();

	const std::vector<Mesh>& getMeshes() const;
	const std::vector<Transform>& getTransforms() const;
	const std::vector<Material>& getMaterials() const;
	const std::vector<Renderer>& getRendererIDs() const;
	const std::vector<VkMesh>& getGraphicsMeshes() const;
	const std::vector<VkMeshRenderer>& getRenderers() const;

	bool load(VkDescriptorPool descPool);
	void release(VkDevice device, VmaAllocator allocator);

	bool tryInitializeFromFile(const Loader::ModelLoaderOptions& modelOptions);
	void createGraphicsRepresentation(VkDescriptorPool descPool);

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);

private:
	const Presentation::Device* m_presentationDevice;
	Presentation::PresentationTarget* m_presentationTarget;

	// Not serialized, serves as intermediary
	std::vector<Mesh> m_meshes;

	// Serialized scene data
	std::vector<Material> m_materials;
	std::vector<Renderer> m_rendererIDs;
	std::vector<Transform> m_transforms;
	std::vector<ProcessedMesh> m_processedMeshes;

	// Graphics data
	std::vector<VkMeshRenderer> m_renderers;
	std::vector<UNQ<VkTexture2D>> m_textures;
	std::vector<UNQ<VkMaterial>> m_graphicsMaterials;
	std::vector<VkMesh> m_graphicsMeshes;
};
 