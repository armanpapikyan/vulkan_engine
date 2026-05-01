#pragma once
#include "Common.h"
#include "pch.h"
#include "VertexBinding.h"
#include "Math/BoundsAABB.h"

struct VkMesh;
class StagingBufferPool;

namespace Presentation {
	class Device;
}
namespace boost {
	namespace serialization {
		class access;
	}
}

struct SubMesh
{
	SubMesh();
	SubMesh(size_t size);
	SubMesh(std::vector<MeshDescriptor::TVertexIndices>& indices);
	size_t getIndexCount() const;

	std::vector<MeshDescriptor::TVertexIndices> m_indices;
	BoundsAABB m_bounds;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);

	bool operator ==(const SubMesh& other) const;
	bool operator !=(const SubMesh& other) const;
};

struct ProcessedMesh
{
	uint32_t getSubmeshCount() const { return as_uint32(m_submeshData.size()); }
	uint32_t getIndexCount(uint32_t index) const { return as_uint32(getSubmesh(index).getIndexCount()); }
	uint32_t getVertCount() const { return m_vertCount; }

	uint32_t getIndexTotalByteSize(uint32_t index) const;
	uint32_t getVertexTotalByteSize() const;

	const void* getIndexMemory(uint32_t index) const { return getSubmesh(index).m_indices.data(); }
	const void* getVertexMemory() const { return m_interleavedVertexData.data(); }

	const BoundsAABB* getBounds(uint32_t index) const { return &getSubmesh(index).m_bounds; }

	void Init(uint64_t hash, size_t vertexCount, size_t interleavedBufferSize, const std::vector<SubMesh>& submeshes);

	void copyInterleavedNoCheck(const void* src, size_t elementByteSize, size_t iterStride, size_t offset);

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);

	bool operator ==(const ProcessedMesh& other) const;
	bool operator !=(const ProcessedMesh& other) const;

private:
	uint64_t m_hash;

	uint32_t m_vertCount;
	std::vector<float> m_interleavedVertexData;
	std::vector<SubMesh> m_submeshData;

	const SubMesh& getSubmesh(uint32_t index) const { return m_submeshData[index]; }
};

struct GraphicsMemoryAllocator
{
	GraphicsMemoryAllocator(const Presentation::Device* presentationDevice, VmaAllocator& vmaAllocator, StagingBufferPool& stagingBufferPool);

	bool createGraphicsMesh(VkMesh& graphicsMesh, const ProcessedMesh& processedMesh);

	void copy(uint32_t totalSizeBytes, const void* source, VkBuffer bufferDestination);

private:

	template<typename T>
	void mapAndCopyBuffer(VmaAllocator vmaAllocator, VmaAllocation memRange, const T* source, size_t totalByteSize)
	{
		void* data;
		vmaMapMemory(vmaAllocator, memRange, &data);
		memcpy(data, source, totalByteSize);
		vmaUnmapMemory(vmaAllocator, memRange);
	}

	StagingBufferPool* m_stagingPool;
	VmaAllocator* m_vmaAllocator;
	const Presentation::Device* m_presentationDevice;
};

struct Mesh
{
public:
	Mesh(const Mesh& mesh);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, std::vector<SubMesh>& submeshes);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh& submesh);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh&& submesh);

	void clear();
	bool isValid() const;

	static bool validateOptionalBufferSize(size_t vectorSize, size_t vertexCount, char const* name);

	bool createProcessedMesh(ProcessedMesh& processedMesh) const;

	void makeFace(glm::vec3 pivot, glm::vec3 up, glm::vec3 right, MeshDescriptor::TVertexIndices firstIndex);
	static Mesh getPrimitiveCube();
	static Mesh getPrimitiveQuad();
	static Mesh getPrimitiveTriangle();

	const MeshDescriptor& getMeshDescriptor() const;
	const BoundsAABB* getBounds(uint32_t submeshIndex) const;

	static MeshDescriptor defaultMeshDescriptor;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);

	bool operator==(const Mesh& other) const;
	bool operator!=(const Mesh& other) const;

private:
	friend class boost::serialization::access;
	Mesh();
	Mesh(size_t vertN, size_t indexN);

	std::vector<MeshDescriptor::TVertexPosition> m_positions;
	std::vector<MeshDescriptor::TVertexUV> m_uvs;
	std::vector<MeshDescriptor::TVertexNormal> m_normals;
	std::vector<MeshDescriptor::TVertexColor> m_colors;

	std::vector<SubMesh> m_submeshes;

	MeshDescriptor m_metaData;
	void* m_vectors[MeshDescriptor::descriptorCount];

	void updateMetaData();
};