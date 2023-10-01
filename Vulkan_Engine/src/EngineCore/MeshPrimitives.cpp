#include "pch.h"
#include "Mesh.h"
#include "Color.h"
#include <glm/ext/quaternion_geometric.hpp>

Mesh Mesh::getPrimitiveQuad()
{
	float extent = 0.5f;

	std::vector<MeshDescriptor::TVertexPosition> positions {
		{ -extent, -extent, 0.0f },
		{ extent, -extent, 0.0f },
		{ -extent, extent, 0.0f },
		{ extent, extent, 0.0f }
	};

	const auto n = positions.size();
	std::vector<MeshDescriptor::TVertexUV> uvs(n);
	std::vector<MeshDescriptor::TVertexNormal> normals(n);
	std::vector<MeshDescriptor::TVertexColor> colors(n);

	if (MeshDescriptor::EAttributePresent::UVs)
	{
		uvs = {
			{ 0.0, 0.0 },
			{ 1.0, 0.0 },
			{ 0.0, 1.0 },
			{ 1.0, 1.0 }
		};
	}

	if (MeshDescriptor::EAttributePresent::Normals)
	{
		glm::vec3 forward(0.0f, 0.0f, 1.0f);
		normals = {
			forward,
			forward,
			forward,
			forward
		};
	}

	if (MeshDescriptor::EAttributePresent::Colors)
	{
		colors = {
			Color::red().v4() * 0.5f,
			Color::green().v4() * 0.5f,
			Color::green().v4() * 0.5f,
			Color::blue().v4() * 0.5f
		};
	}

	std::vector<MeshDescriptor::TVertexIndices> indices {
		0, 1, 2,
		2, 1, 3
	};

	return Mesh(positions, uvs, normals, colors, SubMesh(indices));
}

void Mesh::makeFace(glm::vec3 pivot, glm::vec3 up, glm::vec3 right, MeshDescriptor::TVertexIndices firstIndex)
{
	m_positions.push_back(pivot + up - right);
	m_positions.push_back(pivot + up + right);
	m_positions.push_back(pivot - up - right);
	m_positions.push_back(pivot - up + right);

	if (MeshDescriptor::EAttributePresent::UVs)
	{
		m_uvs.push_back({ 0.0, 1.0 });
		m_uvs.push_back({ 1.0, 1.0 });
		m_uvs.push_back({ 0.0, 0.0 });
		m_uvs.push_back({ 1.0, 0.0 });
	}

	auto normal = glm::normalize(pivot);
	if (MeshDescriptor::EAttributePresent::Normals)
	{
		m_normals.push_back(normal);
		m_normals.push_back(normal);
		m_normals.push_back(normal);
		m_normals.push_back(normal);
	}

	if (MeshDescriptor::EAttributePresent::Colors)
	{
		m_colors.push_back(Color::red().v4());
		m_colors.push_back(Color::green().v4());
		m_colors.push_back(Color::green().v4());
		m_colors.push_back(Color::blue().v4());
	}

	uint16_t indices[6] {
		0, 1, 2,
		2, 1, 3
	};
	m_submeshes.resize(1);
	auto& submesh = m_submeshes[0];

	if (normal.x < 0 || normal.y < 0 || normal.z < 0)
	{
		for (int i = 0; i < 6; i += 1)
			submesh.m_indices.push_back(firstIndex + indices[i]);
	}
	else
	{
		for (int i = 5; i >= 0; i -= 1)
			submesh.m_indices.push_back(firstIndex + indices[i]);
	}
}

Mesh Mesh::getPrimitiveCube()
{
	constexpr MeshDescriptor::TVertexIndices vertPerFace = 4u;
	constexpr MeshDescriptor::TVertexIndices indexPerFace = 6u;
	constexpr MeshDescriptor::TVertexIndices numOfFaces = 6u;
	Mesh mesh(vertPerFace * numOfFaces, indexPerFace * numOfFaces);

	const glm::vec3 right(0.5f, 0.f, 0.f);
	const glm::vec3 up(0.f, 0.5f, 0.f);
	const glm::vec3 forward(0.f, 0.f, 0.5f);

	mesh.makeFace(right, up, -forward, vertPerFace * 0);
	mesh.makeFace(-right, up, -forward, vertPerFace * 1);
	mesh.makeFace(up, right, forward, vertPerFace * 2);
	mesh.makeFace(-up, right, forward, vertPerFace * 3);
	mesh.makeFace(forward, up, right, vertPerFace * 4);
	mesh.makeFace(-forward, up, right, vertPerFace * 5);

	mesh.updateMetaData();
	return mesh;
}

Mesh Mesh::getPrimitiveTriangle()
{
	float extentX = 0.5f;
	float extentY = extentX / 3.0f;

	std::vector<MeshDescriptor::TVertexPosition> positions{
		{ 0.0f, -extentY * 2.f, 0.0f },
		{ extentX, extentY, 0.0f },
		{ -extentX, extentY, 0.0f },
	};

	const auto n = positions.size();
	std::vector<MeshDescriptor::TVertexUV> uvs(n);
	std::vector<MeshDescriptor::TVertexNormal> normals(n);
	std::vector<MeshDescriptor::TVertexColor> colors(n);

	if (MeshDescriptor::EAttributePresent::UVs)
	{
		uvs = {
			{ 0.5, 0 },
			{ 1.0, 1.0 },
			{ 0.0, 1.0 }
		};
	}

	if (MeshDescriptor::EAttributePresent::Normals)
	{
		glm::vec3 forward(0.0f, 0.0f, 1.0f);
		normals = {
			forward,
			forward,
			forward
		};
	}

	if (MeshDescriptor::EAttributePresent::Colors)
	{
		colors = {
			Color::red().v4() * 0.5f + glm::vec4(0.5f),
			Color::green().v4() * 0.5f + glm::vec4(0.5f),
			Color::blue().v4() * 0.5f + glm::vec4(0.5f)
		};
	}

	std::vector<MeshDescriptor::TVertexIndices> indices {
		0, 1, 2
	};

	return Mesh(positions, uvs, normals, colors, indices);
}

const MeshDescriptor& Mesh::getMeshDescriptor() const { return m_metaData; }

const BoundsAABB* Mesh::getBounds(uint32_t submeshIndex) const { return submeshIndex >= 0 && submeshIndex < m_submeshes.size() ? &m_submeshes[submeshIndex].m_bounds : nullptr; }
