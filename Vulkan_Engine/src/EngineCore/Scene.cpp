#include "pch.h"
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
#include "Renderer.h"
#include "Transform.h"
#include "VkTypes/VkMesh.h"
#include "VkTypes/VkTexture.h"
#include "VkTypes/VkShader.h"
#include "VkTypes/VkMeshRenderer.h"

#include "Presentation/Device.h"
#include "Presentation/PresentationTarget.h"
#include "EngineCore/StagingBufferPool.h"

#include "FileManager/FileIO.h"
#include "FileManager/Directories.h"
#include "Profiling/ProfileMarker.h"

#include "Loaders/Model/Common.h"
#include "Loaders/Model/ModelLoaderOptions.h"
#include "Loaders/Model/Loader_OBJ.h"
#include "Loaders/Model/Loader_ASSIMP.h"

Scene::Scene(const Presentation::Device* device, Presentation::PresentationTarget* target)
	: m_presentationDevice(device), m_presentationTarget(target) { }

Scene::~Scene() = default;

constexpr bool force_serialize_from_origin = false;
bool Scene::load(VkDescriptorPool descPool)
{
	//const auto modelOptions = Directories::getModels_DebrovicSponza();
	//const auto modelOptions = Directories::getModels_IntelSponza();
	const auto modelOptions = Directories::getModels_CrytekSponza();

	Path fullPath;
	/*****************************				IMPORT					****************************************/
	if (!Directories::tryGetBinaryIfExists(fullPath, modelOptions.front().filePath) || force_serialize_from_origin)
	{
		ProfileMarker _("Scene::import & serialize");
		Scene scene(nullptr, nullptr);

		for (auto& model : modelOptions)
		{
			auto meshCount = scene.getMeshes().size();
			scene.tryInitializeFromFile(model);
			
			if (scene.getMeshes().size() <= meshCount)
			{
				printf("No meshes loaded from %s.", model.filePath.c_str());
			}
		}

		{
			auto stream = std::fstream(fullPath, std::ios::out | std::ios::binary);
			boost::archive::binary_oarchive archive(stream);

			archive << scene;
			stream.close();
		}
	}

	/*****************************				LOAD BINARY					****************************************/
	ProfileMarker _("Scene::load");

	const Loader::ModelLoaderOptions binaryLoaderOptions(std::move(fullPath), 1.0);
	if (!tryInitializeFromFile(binaryLoaderOptions))
	{
		printf("Could not load scene file '%s', it did not match any of the supported formats.\n", binaryLoaderOptions.filePath.c_str());
		return false;
	}
	printf("Initialized the scene with (renderers = %zi), (transforms = %zi), (meshes = %zi), (textures = %zi), (materials = %zi).\n", m_renderers.size(), m_transforms.size(), m_meshes.size(), m_textures.size(), m_materials.size());

	/*****************************				GRAPHICS					****************************************/
	createGraphicsRepresentation(descPool);

	return true;
}

const std::vector<Mesh>& Scene::getMeshes() const { return m_meshes; }
const std::vector<Transform>& Scene::getTransforms() const { return m_transforms; }
const std::vector<Material>& Scene::getMaterials() const { return m_materials; }
const std::vector<Renderer>& Scene::getRendererIDs() const { return m_rendererIDs; }
const std::vector<VkMesh>& Scene::getGraphicsMeshes() const { return m_graphicsMeshes; }
const std::vector<VkMeshRenderer>& Scene::getRenderers() const { return m_renderers; }

void Scene::release(VkDevice device, VmaAllocator allocator)
{
	for (auto& mesh : m_meshes)
	{
		mesh.clear();
	}
	m_meshes.clear();

	for (auto& gmesh : m_graphicsMeshes)
	{
		gmesh.release(allocator);
	}
	m_graphicsMeshes.clear();

	for (auto& tex : m_textures)
	{
		tex->release(device);
		tex = nullptr;
	}
	m_textures.clear();

	for (auto& mat : m_graphicsMaterials)
	{
		mat->release(device);
		mat = nullptr;
	}
	m_graphicsMaterials.clear();

	m_renderers.clear();
	m_materials.clear();
	m_rendererIDs.clear();
}

template<class Archive>
void Scene::serialize(Archive& ar, const unsigned int version)
{
	ProfileMarker _("Scene::Serialize");

	ar& m_materials;
	ar& m_rendererIDs;
	ar& m_transforms;
	ar& m_processedMeshes;
}

template<class Archive>
void SubMesh::serialize(Archive& ar, const unsigned int version)
{
	ar& m_indices;
	ar& m_bounds.center;
	ar& m_bounds.extents;
}

template<class Archive>
inline void Mesh::serialize(Archive& ar, const unsigned int version)
{
	ar& m_positions;
	ar& m_uvs;
	ar& m_normals;
	ar& m_colors;
	ar& m_submeshes;

	updateMetaData();
}

template<class Archive>
void Renderer::serialize(Archive& ar, const unsigned int version)
{
	ar& meshID;
	ar& materialIDs;
	ar& transformID;
}

template<class Archive>
void ProcessedMesh::serialize(Archive& ar, const unsigned int version)
{
	ar& m_hash;
	ar& m_vertCount;
	ar& m_interleavedVertexData;
	ar& m_submeshData;
}

bool Scene::tryInitializeFromFile(const Loader::ModelLoaderOptions& modelOptions)
{
	const auto& path = modelOptions.filePath;

	/* ================ READ SERIALIZED BINARY =============== */
	if (Directories::isBinary(path) && path.fileExists())
	{
		ProfileMarker _("Loader::Binary");

		// Deserialize
		auto stream = std::fstream(path, std::ios::in | std::ios::binary);
		boost::archive::binary_iarchive archive(stream);

		archive >> *this;

		return true;
	}

	bool parsedSuccessful = false;
	/* ================ READ FROM GLTF =============== */
	if (FileIO::fileExists(path, ".gltf"))
	{
		ProfileMarker _("Loader::ASSIMP");
		parsedSuccessful = Loader::load_AssimpImplementation(m_meshes, m_materials, m_rendererIDs, m_transforms, modelOptions);
	}

	/* ================ READ FROM OBJ =============== */
	if (!parsedSuccessful && FileIO::fileExists(path, ".obj"))
	{
		ProfileMarker _("Loader::Custom_OBJ");
		parsedSuccessful = Loader::loadOBJ_Implementation(m_meshes, m_materials, m_rendererIDs, m_transforms, modelOptions);
	}

	// Fallback
	if (!parsedSuccessful && FileIO::fileExists(path))
	{
		ProfileMarker _("Loader::ASSIMP");
		parsedSuccessful = Loader::load_AssimpImplementation(m_meshes, m_materials, m_rendererIDs, m_transforms, modelOptions);
	}

	if (parsedSuccessful)
	{
		m_processedMeshes.resize(m_meshes.size());
		for (size_t i = 0, n = m_meshes.size(); i < n; i++)
		{
			m_meshes[i].createProcessedMesh(m_processedMeshes[i]);
		}
	}

	return parsedSuccessful;
}

void Scene::createGraphicsRepresentation(VkDescriptorPool descPool)
{
	std::unordered_set<const TextureSource*> texturesToLoad{};
	std::unordered_map<const TextureSource*, const VkMaterial*> createdGraphicsMaterials{};

	uint32_t submeshCount = 0;
	StagingBufferPool stagingBufPool{};
	{
		ProfileMarker _("Scene::Create_Graphics_Materials");
		/* ================= CREATE TEXTURES ================*/
		/* ================= CREATE GRAPHICS MATERIALS ================*/
		auto device = m_presentationDevice->getDevice();
		for (auto& rendererIDs : m_rendererIDs)
		{
			for (auto& matIndex : rendererIDs.materialIDs)
			{
				const Material& material = m_materials[matIndex];
				texturesToLoad.insert(&material.getTextureSource());

				submeshCount += 1;
			}
		}
		m_textures.resize(texturesToLoad.size());
		m_graphicsMaterials.resize(texturesToLoad.size());

		uint32_t index = 0;
		for (const TextureSource* source : texturesToLoad)
		{
			UNQ<VkTexture2D>& texture = m_textures[index];
			if (VkTexture2D::tryCreateTexture(texture, *source, m_presentationDevice, stagingBufPool))
			{
				auto* shader = VkShader::findShader(0);
				m_presentationTarget->createGraphicsMaterial(m_graphicsMaterials[index], device, descPool, shader, texture.get());
				createdGraphicsMaterials[source] = m_graphicsMaterials[index].get();

				index += 1;
			}
			else
			{
				printf("The texture at '%s' could not be loaded.\n", source->path.c_str());
			}
		}
	}

	{
		ProfileMarker _("Scene::Create_Graphics_Meshes");
		auto vmaAllocator = VkMemoryAllocator::getInstance()->m_allocator;
		GraphicsMemoryAllocator alloc(m_presentationDevice, vmaAllocator, stagingBufPool);

		/* ================= CREATE GRAPHICS MESHES ================*/
		const auto& defaultMeshDescriptor = Mesh::defaultMeshDescriptor;
		const auto count = m_processedMeshes.size();

		m_graphicsMeshes.reserve(count);
		m_renderers.reserve(submeshCount);
		for (auto& ids : m_rendererIDs)
		{
			const auto& processedMesh = m_processedMeshes[ids.meshID];

			auto newGraphicsMesh = VkMesh();
			if (!alloc.createGraphicsMesh(newGraphicsMesh, processedMesh))
				continue;

			m_graphicsMeshes.emplace_back(std::move(newGraphicsMesh));

			uint32_t submeshIndex = 0;
			for (auto materialIDs : ids.materialIDs)
			{
				const auto* texPath = &m_materials[materialIDs].getTextureSource();
				if (createdGraphicsMaterials.count(texPath) == 0)
				{
					printf("SceneLoader - The material '%s' not found for '%zu' mesh.\n", texPath->path.c_str(), ids.meshID);
					continue;
				}

				m_renderers.emplace_back(
					&m_graphicsMeshes.back(), submeshIndex, &m_materials[materialIDs],
					&createdGraphicsMaterials[texPath]->getMaterialVariant(),
					processedMesh.getBounds(submeshIndex), &m_transforms[ids.transformID]
				);
				++submeshIndex;
			}

		}
	}
	stagingBufPool.releaseAllResources();
}
