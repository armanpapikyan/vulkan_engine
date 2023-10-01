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

Mesh::Mesh() : m_vectors(), m_metaData() {}

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

bool Mesh::createProcessedMesh(ProcessedMesh& processedMesh)
{
	size_t vertCount = m_positions.size();

	constexpr size_t descriptorCount = MeshDescriptor::descriptorCount;
	auto& byteSizes = m_metaData.elementByteSizes;
	auto& vectorSizes = m_metaData.lengths;

	size_t strides[descriptorCount];
	for (int i = 0; i < descriptorCount; i++)
		strides[i] = std::clamp(m_metaData.lengths[i], 0_z, 1_z) * byteSizes[i];

	size_t vertexStride = sumValues(strides, descriptorCount);
	size_t floatsPerVert = vertexStride / sizeof(float);
	size_t totalSizeBytes = vertexStride * vertCount;

	// Align and optimize the mesh data
	auto interleavedCount = floatsPerVert * vertCount;
	processedMesh.Init(
		0, // HASH
		vertCount, interleavedCount, m_submeshes);

	size_t offset = 0;
	for (int i = 0; i < descriptorCount; i++)
	{
		if (vectorSizes[i] == 0)
			continue;

		processedMesh.copyInterleavedNoCheck(m_vectors[i], byteSizes[i], floatsPerVert, offset);
		offset += strides[i] / sizeof(float);
	}

	return true;
}

void Mesh::updateMetaData()
{
	m_vectors[0] = m_positions.data();
	m_vectors[1] = m_uvs.data();
	m_vectors[2] = m_normals.data();
	m_vectors[3] = m_colors.data();

	m_metaData.lengths[0] = m_positions.size();
	m_metaData.lengths[1] = m_uvs.size();
	m_metaData.lengths[2] = m_normals.size();
	m_metaData.lengths[3] = m_colors.size();

	m_metaData.elementByteSizes[0] = vectorElementsizeof(m_positions);
	m_metaData.elementByteSizes[1] = vectorElementsizeof(m_uvs);
	m_metaData.elementByteSizes[2] = vectorElementsizeof(m_normals);
	m_metaData.elementByteSizes[3] = vectorElementsizeof(m_colors);
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

GraphicsMemoryAllocator::GraphicsMemoryAllocator(const Presentation::Device* presentationDevice, VmaAllocator& vmaAllocator, StagingBufferPool& stagingBufferPool)
	: m_stagingPool(&stagingBufferPool), m_vmaAllocator(&vmaAllocator), m_presentationDevice(presentationDevice) { }

bool GraphicsMemoryAllocator::createGraphicsMesh(VkMesh& graphicsMesh, const ProcessedMesh& processedMesh)
{
	VkBuffer buffer;
	VmaAllocation memRange;

	VkIndexType indexPrecision = VkIndexType::VK_INDEX_TYPE_MAX_ENUM;
	if (sizeof(MeshDescriptor::TVertexIndices) == sizeof(uint16_t))
		indexPrecision = VkIndexType::VK_INDEX_TYPE_UINT16;
	if (sizeof(MeshDescriptor::TVertexIndices) == sizeof(uint32_t))
		indexPrecision = VkIndexType::VK_INDEX_TYPE_UINT32;
	assert(indexPrecision != VkIndexType::VK_INDEX_TYPE_MAX_ENUM);

	// Copy verts
	if (!vkinit::MemoryBuffer::allocateBufferAndMemory(buffer, memRange, *m_vmaAllocator, processedMesh.getVertexTotalByteSize(), VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT))
	{
		printf("Could not allocate vertex memory buffer.\n");
		return false;
	}
	
	copy(processedMesh.getVertexTotalByteSize(), processedMesh.getVertexMemory(), buffer);

	graphicsMesh.vCount = processedMesh.getVertCount();
	graphicsMesh.vAttributes = MAKEUNQ<VertexAttributes>(buffer, memRange, 0);

	// Copy indices
	for (int i = 0, n = processedMesh.getSubmeshCount(); i < n; i++)
	{
		VkBuffer buffer;
		VmaAllocation memRange;

		auto totalByteSize = processedMesh.getIndexTotalByteSize(i);
		if (!vkinit::MemoryBuffer::allocateBufferAndMemory(buffer, memRange, *m_vmaAllocator, totalByteSize, VMA_MEMORY_USAGE_GPU_ONLY, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT))
		{
			printf("Could not allocate vertex memory buffer.\n");
			return false;
		}

		copy(totalByteSize, processedMesh.getIndexMemory(i), buffer);

		graphicsMesh.iAttributes.emplace_back(
			buffer, memRange, processedMesh.getIndexCount(i), indexPrecision
		);
	}

	return true;
}

void GraphicsMemoryAllocator::copy(uint32_t totalSizeBytes, const void* source, VkBuffer bufferDestination)
{
	// Copy to staging buffer
	StagingBufferPool::StgBuffer stagingBuffer;
	m_stagingPool->claimAStagingBuffer(stagingBuffer, totalSizeBytes);
	mapAndCopyBuffer(*m_vmaAllocator, stagingBuffer.allocation, source, totalSizeBytes);

#ifdef VERBOSE_INFO_MESSAGE
	elementCount = -1;
	printf("Copied vertex buffer of size: %zu elements and %zu bytes (%f bytes per vertex).\n", elementCount, totalByteSize, totalByteSize / static_cast<float>(elementCount));

#endif

	// Copy from staging buffer to permanent buffer
	m_presentationDevice->submitImmediatelyAndWaitCompletion([=](VkCommandBuffer cmd) {
		VkBufferCopy copyRegion{};
		copyRegion.size = totalSizeBytes;

		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, bufferDestination, 1, &copyRegion);
	});

	// Release staging buffer back to the pool
	m_stagingPool->freeBuffer(stagingBuffer);
}

uint32_t ProcessedMesh::getIndexTotalByteSize(int index) const { return vectorsizeof(getSubmesh(index).m_indices); }

uint32_t ProcessedMesh::getVertexTotalByteSize() const { return vectorsizeof(m_interleavedVertexData); }
