#pragma once
#include "pch.h"
#include "VertexBinding.h"
#include "VkTypes/VkShader.h"
#include "InitializersUtility.h"
#include "CollectionUtility.h"

namespace PipelineConstruction
{
	struct ComponentCreateInfoAbstract
	{
		ComponentCreateInfoAbstract();
		~ComponentCreateInfoAbstract();

		virtual bool isValid() const = 0;
		virtual void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const = 0;

		void submitIfValid(VkGraphicsPipelineCreateInfo& pipelineCI);
	};

	template <typename T>
	struct ComponentCI : ComponentCreateInfoAbstract
	{
	protected:
		T m_createInfo{};
	};

	struct GraphicsPipelineCI : ComponentCreateInfoAbstract
	{
		static constexpr VkStructureType STRUCTURE_TYPE = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		GraphicsPipelineCI(VkPipelineLayout pipelineLayout);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

		VkPipelineLayout m_pipelineLayout;
	};

	struct RenderPass : ComponentCreateInfoAbstract
	{
		RenderPass(VkRenderPass renderPass);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

	private:
		VkRenderPass m_renderPass;
	};

	struct ShaderStage : ComponentCI<VkPipelineShaderStageCreateInfo>
	{
		enum class SupportedStages
		{
			Vertex = VK_SHADER_STAGE_VERTEX_BIT,
			Fragment = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		ShaderStage(VkShaderModule shader, SupportedStages stage, const char* fnName = nullptr);

		bool isValid() const override;
		const VkPipelineShaderStageCreateInfo getCreateInfo() const;
	private:
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;
	};

	struct PipelineStageCollection : ComponentCreateInfoAbstract
	{
		PipelineStageCollection(ShaderStage* vertStage, ShaderStage* fragStage);
		~PipelineStageCollection() = default;

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

		std::array<VkPipelineShaderStageCreateInfo, 2> m_allStages;
		uint32_t m_stageCount;
	};

	struct InputAssembly : ComponentCI<VkPipelineInputAssemblyStateCreateInfo>
	{
		InputAssembly();

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;
	};

	enum FaceCulling
	{
		None = VK_CULL_MODE_NONE,
		Front = VK_CULL_MODE_FRONT_BIT,
		Back = VK_CULL_MODE_BACK_BIT,
		Both = VK_CULL_MODE_FRONT_AND_BACK,
	};

	enum TriangleWinding
	{
		CCW = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		CW = VK_FRONT_FACE_CLOCKWISE
	};

	enum PolygonMode
	{
		Fill = VK_POLYGON_MODE_FILL,
		Line = VK_POLYGON_MODE_LINE,
		Point = VK_POLYGON_MODE_POINT
	};
	struct RasterizationState : ComponentCI<VkPipelineRasterizationStateCreateInfo>
	{
		RasterizationState(FaceCulling faceCullingMode, TriangleWinding winding = TriangleWinding::CCW, PolygonMode polygoneMode = PolygonMode::Fill);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;
	};

	struct MultisampleState : ComponentCI<VkPipelineMultisampleStateCreateInfo>
	{
		MultisampleState();

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;
	};

	struct ColorBlendState : ComponentCI<VkPipelineColorBlendStateCreateInfo>
	{
		ColorBlendState();

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

	private:
		VkPipelineColorBlendAttachmentState m_attachmentState;
	};

	struct DepthStencilState : ComponentCI<VkPipelineDepthStencilStateCreateInfo>
	{
		DepthStencilState(bool enabled);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;
	};

	struct ViewportState : ComponentCI<VkPipelineViewportStateCreateInfo>
	{
		ViewportState(VkExtent2D extent);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

	private:
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

	struct DynamicState : ComponentCI<VkPipelineDynamicStateCreateInfo>
	{
		DynamicState();

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

	private:
		VkDynamicState m_dynamicStates[2];
	};

	struct VertexInputState : ComponentCI<VkPipelineVertexInputStateCreateInfo>
	{
		VertexInputState(const MeshDescriptor* meshDescriptor);

		bool isValid() const override;
		void submit(VkGraphicsPipelineCreateInfo& pipelineCI) const override;

	private:
		VkVertexInputBindingDescription bindingDescription;
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

		bool m_isValid;
	};

	bool createPipeline(VkPipeline& pipelineInstance, const VkPipelineLayout pipelineLayout, const VkDevice device, const VkRenderPass renderPass,
		VkExtent2D swapchainExtent, const VkShader& shader, const MeshDescriptor* descriptor, FaceCulling faceCullingMode, bool depthStencilAttachement);
}