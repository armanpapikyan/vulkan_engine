#pragma once
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

struct Mesh
{
public:
	Mesh(const Mesh& mesh);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, std::vector<SubMesh>& submeshes);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh& submesh);
	Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh&& submesh);

	void clear();
	bool isValid();

	template<typename T>
	void mapAndCopyBuffer(VmaAllocator vmaAllocator, VmaAllocation memRange, const T* source, size_t elementCount, size_t totalByteSize, const char* message);

	static bool validateOptionalBufferSize(size_t vectorSize, size_t vertexCount, char const* name);
	static void copyInterleavedNoCheck(std::vector<float>& interleavedVertexData, const void* src, size_t elementByteSize, size_t iterStride, size_t offset);

	bool allocateGraphicsMesh(VkMesh& graphicsMesh, VmaAllocator vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool);
	bool allocateIndexAttributes(VkMesh& graphicsMesh, const SubMesh& submesh, VmaAllocator vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool);
	bool allocateVertexAttributes(VkMesh& graphicsMesh, const VmaAllocator& vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool);

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

	MeshDescriptor metaData;
	void* vectors[MeshDescriptor::descriptorCount];

	void updateMetaData();
};