#include "pch.h"
#include "Common.h"
#include "CollectionUtility.h"
#include "VkTypes/VkMesh.h"
#include "Mesh.h"
#include "VertexAttributes.h"
#include "Presentation/Device.h"
#include "StagingBufferPool.h"

MeshDescriptor Mesh::defaultMeshDescriptor = MeshDescriptor();

Mesh::Mesh(const Mesh& mesh) : m_positions(mesh.m_positions), m_uvs(mesh.m_uvs), m_normals(mesh.m_normals), m_colors(mesh.m_colors), m_submeshes(mesh.m_submeshes) { updateMetaData(); }

Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, std::vector<SubMesh>& submeshes)
	: m_positions(std::move(positions)), m_uvs(std::move(uvs)), m_normals(std::move(normals)), m_colors(std::move(colors)), m_submeshes(std::move(submeshes))
{
	updateMetaData();
}

Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh& submesh)
	: m_positions(std::move(positions)), m_uvs(std::move(uvs)), m_normals(std::move(normals)), m_colors(std::move(colors)), m_submeshes(1)
{
	m_submeshes[0] = std::move(submesh);
	updateMetaData();
}

Mesh::Mesh(std::vector<glm::vec3>& positions, std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals, std::vector<glm::vec3>& colors, SubMesh&& submesh)
	: m_positions(std::move(positions)), m_uvs(std::move(uvs)), m_normals(std::move(normals)), m_colors(std::move(colors)), m_submeshes(1)
{
	m_submeshes[0] = std::move(submesh);
	updateMetaData();
}

Mesh::Mesh() : vectors(), metaData() {}

Mesh::Mesh(size_t vertN, size_t indexN)
{
	m_positions.reserve(vertN);
	m_uvs.reserve(vertN);
	m_normals.reserve(vertN);
	m_colors.reserve(vertN);

	m_submeshes.resize(1);
	m_submeshes[0].m_indices.reserve(indexN);

	updateMetaData();
}

void Mesh::clear()
{
	m_positions.clear();
	m_uvs.clear();
	m_normals.clear();
	m_colors.clear();

	m_submeshes.clear();
}

bool Mesh::validateOptionalBufferSize(size_t vectorSize, size_t vertexCount, char const* name)
{
	// Optional vector, but if it exists the size should match the position array.
	if (vectorSize > 0 && vectorSize != vertexCount)
	{
		printf("%s array size '%zu' does not match the position array size '%zu'.\n", name, vectorSize, vertexCount);
		return false;
	}

	return true;
}

bool Mesh::isValid()
{
	const auto n = m_positions.size();

	if (n == 0)
	{
		printf("The vertex array can not have 0 length.\n");
		return false;
	}

	if (n >= std::numeric_limits<MeshDescriptor::TVertexIndices>::max())
	{
		printf("The vertex array size '%zu' exceeds the allowed capacity '%i'.\n", n, std::numeric_limits<MeshDescriptor::TVertexIndices>::max());
		return false;
	}

	if (!validateOptionalBufferSize(m_uvs.size(), n, "Uvs") ||
		!validateOptionalBufferSize(m_normals.size(), n, "Normals") ||
		!validateOptionalBufferSize(m_colors.size(), n, "Colors"))
		return false;

#ifndef NDEBUG
	for (auto& submesh : m_submeshes)
	{
		for (size_t i = 0, size = submesh.m_indices.size(); i < size; ++i)
		{
			auto index = submesh.m_indices[i];
			if (index < 0 || index >= n)
			{
				printf("An incorrect index '%i' detected at position indices[%zu], should be in {0, %zu} range.\n", index, i, n);
				return false;
			}
		}
	}
#endif

	return true;
}

bool MeshDescriptor::operator ==(const MeshDescriptor& other) const
{
	for (int i = 0; i < descriptorCount; i++)
	{
		if (elementByteSizes[i] != other.elementByteSizes[i] ||
			std::min(lengths[i], 1_z) != std::min(other.lengths[i], 1_z))
			return false;
	}
	return true;
}

bool MeshDescriptor::operator !=(const MeshDescriptor& other) const { return !(*this == other); }

template<typename T>
inline void Mesh::mapAndCopyBuffer(VmaAllocator vmaAllocator, VmaAllocation memRange, const T* source, size_t elementCount, size_t totalByteSize, const char* message)
{
	void* data;
	vmaMapMemory(vmaAllocator, memRange, &data);
	memcpy(data, source, totalByteSize);
	vmaUnmapMemory(vmaAllocator, memRange);

#ifdef VERBOSE_INFO_MESSAGE
	printf(message, elementCount, totalByteSize, totalByteSize / static_cast<float>(elementCount));
#endif
}

bool Mesh::allocateVertexAttributes(VkMesh& graphicsMesh, const VmaAllocator& vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool)
{
	size_t vertCount = m_positions.size();

	constexpr size_t descriptorCount = MeshDescriptor::descriptorCount;
	auto& byteSizes = metaData.elementByteSizes;
	auto& vectorSizes = metaData.lengths;

	size_t strides[descriptorCount];
	for (int i = 0; i < descriptorCount; i++)
		strides[i] = std::clamp(metaData.lengths[i], 0_z, 1_z) * byteSizes[i];

	size_t vertexStride = sumValues(strides, descriptorCount);
	size_t floatCount = vertexStride / sizeof(float);
	size_t totalSizeBytes = vertexStride * vertCount;
	// Align and optimize the mesh data
	auto interleavedCount = floatCount * vertCount;
	std::vector<float> interleavedVertexData(interleavedCount);

	size_t offset = 0;
	for (int i = 0; i < descriptorCount; i++)
	{
		if (vectorSizes[i] == 0)
			continue;

		copyInterleavedNoCheck(interleavedVertexData, vectors[i], byteSizes[i], floatCount, offset);
		offset += strides[i] / sizeof(float);
	}

	// Copy to staging buffer
	StagingBufferPool::StgBuffer stagingBuffer;
	stagingPool.claimAStagingBuffer(stagingBuffer, as_uint32(totalSizeBytes));
	mapAndCopyBuffer(vmaAllocator, stagingBuffer.allocation, interleavedVertexData.data(), vertCount, totalSizeBytes,
		"Copied vertex buffer of size: %zu elements and %zu bytes (%f bytes per vertex).\n");

	// Allocate permanent buffer
	VkBuffer vBuffer;
	VmaAllocation vMemRange;
	if (!vkinit::MemoryBuffer::allocateBufferAndMemory(vBuffer, vMemRange, vmaAllocator, as_uint32(totalSizeBytes), VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT))
	{
		printf("Could not allocate vertex memory buffer.\n");
		return false;
	}

	// Copy from staging buffer to permanent buffer
	presentationDevice->submitImmediatelyAndWaitCompletion([=](VkCommandBuffer cmd) {
		VkBufferCopy copyRegion{};
		copyRegion.size = as_uint32(totalSizeBytes);

		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, vBuffer, 1, &copyRegion);
	});
	// Release staging buffer back to the pool
	stagingPool.freeBuffer(stagingBuffer);

	std::vector<VkBuffer> vBuffers{ vBuffer };
	std::vector<VkDeviceSize> vOffsets{ 0 };
	std::vector<VmaAllocation> vMemRanges{ vMemRange };
	graphicsMesh.vAttributes = MAKEUNQ<VertexAttributes>(vBuffers, vMemRanges, vOffsets);
	graphicsMesh.vCount = as_uint32(vertCount);

	return true;
}

bool Mesh::allocateIndexAttributes(VkMesh& graphicsMesh, const SubMesh& submesh, VmaAllocator vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool)
{
	size_t totalSize = vectorsizeof(submesh.m_indices);
	auto indexCount = submesh.getIndexCount();

	// Copy to staging buffer
	StagingBufferPool::StgBuffer stagingBuffer;
	stagingPool.claimAStagingBuffer(stagingBuffer, as_uint32(totalSize));

	// Fill the index buffer
	mapAndCopyBuffer(vmaAllocator, stagingBuffer.allocation, submesh.m_indices.data(), indexCount / 3, totalSize,
		"Copied index buffer of size: %zu triangles and %zu bytes (%f bytes per triangle).\n");

	// Allocate permanent buffer
	VkBuffer iBuffer;
	VmaAllocation iMemRange;
	if (!vkinit::MemoryBuffer::allocateBufferAndMemory(iBuffer, iMemRange, vmaAllocator, as_uint32(totalSize), VMA_MEMORY_USAGE_CPU_TO_GPU, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT))
	{
		printf("Could not allocate index memory buffer.\n");
		return false;
	}

	// Copy from staging buffer to permanent buffer
	presentationDevice->submitImmediatelyAndWaitCompletion([=](VkCommandBuffer cmd) {
		VkBufferCopy copyRegion{};
		copyRegion.size = as_uint32(totalSize);

		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, iBuffer, 1, &copyRegion);
	});
	// Release staging buffer back to the pool
	stagingPool.freeBuffer(stagingBuffer);

	VkIndexType indexPrecision = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	if (sizeof(MeshDescriptor::TVertexIndices) == sizeof(uint16_t))
		indexPrecision = VkIndexType::VK_INDEX_TYPE_UINT16;
	if(sizeof(MeshDescriptor::TVertexIndices) == sizeof(uint32_t)) 
		indexPrecision = VkIndexType::VK_INDEX_TYPE_UINT32;
	assert(indexPrecision != VkIndexType::VK_INDEX_TYPE_MAX_ENUM);

	graphicsMesh.iAttributes.emplace_back(
		iBuffer, iMemRange, as_uint32(indexCount), indexPrecision
	);

	return true;
}

void Mesh::copyInterleavedNoCheck(std::vector<float>& interleavedVertexData, const void* src, size_t elementByteSize, size_t iterStride, size_t offset)
{
	int i = 0;
	auto it = interleavedVertexData.begin();
	auto end = interleavedVertexData.end();
	std::advance(it, offset);

	while (it != end)
	{
		memcpy(&(*it), (char*)src + i * elementByteSize, elementByteSize);
		if (std::distance(it, end) < static_cast<ptrdiff_t>(iterStride))
			break;

		std::advance(it, iterStride);
		++i;
	}
}

bool Mesh::allocateGraphicsMesh(VkMesh& graphicsMesh, VmaAllocator vmaAllocator, const Presentation::Device* presentationDevice, StagingBufferPool& stagingPool)
{
	if (!allocateVertexAttributes(graphicsMesh, vmaAllocator, presentationDevice, stagingPool))
		return false;

	for (auto& submesh : m_submeshes)
	{
		if (!allocateIndexAttributes(graphicsMesh, submesh, vmaAllocator, presentationDevice, stagingPool))
			return false;
	}

	return true;
}

void Mesh::updateMetaData()
{
	vectors[0] = m_positions.data();
	vectors[1] = m_uvs.data();
	vectors[2] = m_normals.data();
	vectors[3] = m_colors.data();

	metaData.lengths[0] = m_positions.size();
	metaData.lengths[1] = m_uvs.size();
	metaData.lengths[2] = m_normals.size();
	metaData.lengths[3] = m_colors.size();

	metaData.elementByteSizes[0] = vectorElementsizeof(m_positions);
	metaData.elementByteSizes[1] = vectorElementsizeof(m_uvs);
	metaData.elementByteSizes[2] = vectorElementsizeof(m_normals);
	metaData.elementByteSizes[3] = vectorElementsizeof(m_colors);
}

bool Mesh::operator==(const Mesh& other) const
{
	if (m_positions.size() != other.m_positions.size() ||
		m_uvs.size() != other.m_uvs.size() ||
		m_normals.size() != other.m_normals.size() ||
		m_colors.size() != other.m_colors.size() ||
		m_submeshes.size() != other.m_submeshes.size())
		return false;

	for (size_t i = 0; i < m_submeshes.size(); i++)
	{
		if (m_submeshes[i] != other.m_submeshes[i])
			return false;
	}

	const auto EPSILON3 = glm::vec3(1e-6f);
	for (size_t i = 0; i < m_positions.size(); i++)
	{
		auto res = glm::epsilonNotEqual(m_positions[i], other.m_positions[i], EPSILON3);
		if (res.x || res.y || res.z)
			return false;
	}

	const auto EPSILON2 = glm::vec2(1e-6f);
	for (size_t i = 0; i < m_uvs.size(); i++)
	{
		auto res = glm::epsilonNotEqual(m_uvs[i], other.m_uvs[i], EPSILON2);
		if (res.x || res.y)
			return false;
	}

	for (size_t i = 0; i < m_normals.size(); i++)
	{
		auto res = glm::epsilonNotEqual(m_normals[i], other.m_normals[i], EPSILON3);
		if (res.x || res.y || res.z)
			return false;
	}

	for (size_t i = 0; i < m_colors.size(); i++)
	{
		auto res = glm::epsilonNotEqual(m_colors[i], other.m_colors[i], EPSILON3);
		if (res.x || res.y || res.z)
			return false;
	}

	return true;
}

bool Mesh::operator!=(const Mesh& other) const { return !(*this == other); }