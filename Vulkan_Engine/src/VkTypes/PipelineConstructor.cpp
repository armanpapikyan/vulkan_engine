#include "pch.h"
#include "PipelineConstructor.h"

PipelineConstruction::ComponentCreateInfoAbstract::ComponentCreateInfoAbstract() { }
PipelineConstruction::ComponentCreateInfoAbstract::~ComponentCreateInfoAbstract() { }
void PipelineConstruction::ComponentCreateInfoAbstract::submitIfValid(VkGraphicsPipelineCreateInfo& pipelineCI)
{
	if (isValid())
	{
		submit(pipelineCI);
	}
}

PipelineConstruction::GraphicsPipelineCI::GraphicsPipelineCI(VkPipelineLayout pipelineLayout) : m_pipelineLayout(pipelineLayout) { }
bool PipelineConstruction::GraphicsPipelineCI::isValid() const { return true; }
void PipelineConstruction::GraphicsPipelineCI::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.sType = STRUCTURE_TYPE;
	pipelineCI.layout = m_pipelineLayout;

	// Parent pipeline
	pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCI.basePipelineIndex = -1;
}

PipelineConstruction::RenderPass::RenderPass(VkRenderPass renderPass) : m_renderPass(renderPass) { }
bool PipelineConstruction::RenderPass::isValid() const { return true; }
void PipelineConstruction::RenderPass::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.renderPass = m_renderPass;
}

PipelineConstruction::ShaderStage::ShaderStage(VkShaderModule shader, SupportedStages stage, const char* fnName)
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_createInfo.stage = static_cast<VkShaderStageFlagBits>(stage);
	m_createInfo.module = shader;
	m_createInfo.pName = fnName ? fnName : "main";
}
bool PipelineConstruction::ShaderStage::isValid() const { return m_createInfo.module != VK_NULL_HANDLE; }
void PipelineConstruction::ShaderStage::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const { throw std::exception("Should not call submit on ShaderStage."); }
const VkPipelineShaderStageCreateInfo PipelineConstruction::ShaderStage::getCreateInfo() const { return m_createInfo; }

PipelineConstruction::PipelineStageCollection::PipelineStageCollection(ShaderStage* vertStage, ShaderStage* fragStage)
	: m_allStages(), m_stageCount(0)
{
	if (vertStage && vertStage->isValid())
	{
		m_allStages[m_stageCount] = vertStage->getCreateInfo();
		m_stageCount += 1;
	}

	if (fragStage && fragStage->isValid())
	{
		m_allStages[m_stageCount] = fragStage->getCreateInfo();
		m_stageCount += 1;
	}
}
bool PipelineConstruction::PipelineStageCollection::isValid() const { return m_stageCount > 0; }
void PipelineConstruction::PipelineStageCollection::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	assert(m_stageCount <= m_allStages.size());

	pipelineCI.stageCount = m_stageCount;
	pipelineCI.pStages = m_allStages.data();
}

PipelineConstruction::InputAssembly::InputAssembly()
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_createInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_createInfo.primitiveRestartEnable = VK_FALSE;
}
bool PipelineConstruction::InputAssembly::isValid() const { return true; }
void PipelineConstruction::InputAssembly::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pInputAssemblyState = &m_createInfo;
}

PipelineConstruction::RasterizationState::RasterizationState(FaceCulling faceCullingMode, TriangleWinding winding, PolygonMode polygoneMode)
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_createInfo.depthClampEnable = VK_FALSE;
	m_createInfo.rasterizerDiscardEnable = VK_FALSE;
	m_createInfo.polygonMode = static_cast<VkPolygonMode>(polygoneMode);
	m_createInfo.lineWidth = 1.0f;
	m_createInfo.cullMode = static_cast<VkCullModeFlagBits>(faceCullingMode);
	m_createInfo.frontFace = static_cast<VkFrontFace>(winding);

	m_createInfo.depthBiasEnable = VK_FALSE;
	m_createInfo.depthBiasConstantFactor = 0.0f;
	m_createInfo.depthBiasClamp = 0.0f;
	m_createInfo.depthBiasSlopeFactor = 0.0f;
}
bool PipelineConstruction::RasterizationState::isValid() const { return true; }
void PipelineConstruction::RasterizationState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pRasterizationState = &m_createInfo;
}

PipelineConstruction::MultisampleState::MultisampleState()
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_createInfo.sampleShadingEnable = VK_FALSE;
	m_createInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_createInfo.minSampleShading = 1.0f;
	m_createInfo.pSampleMask = nullptr;
	m_createInfo.alphaToCoverageEnable = VK_FALSE;
	m_createInfo.alphaToOneEnable = VK_FALSE;
}
bool PipelineConstruction::MultisampleState::isValid() const { return true; }
void PipelineConstruction::MultisampleState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pMultisampleState = &m_createInfo;
}

PipelineConstruction::ColorBlendState::ColorBlendState()
{
	m_attachmentState = {};
	m_attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_attachmentState.blendEnable = VK_FALSE;
	m_attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	m_attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_createInfo.logicOpEnable = VK_FALSE;
	m_createInfo.logicOp = VK_LOGIC_OP_COPY;
	m_createInfo.attachmentCount = 1;
	m_createInfo.pAttachments = &m_attachmentState;
}
bool PipelineConstruction::ColorBlendState::isValid() const { return true; }
void PipelineConstruction::ColorBlendState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pColorBlendState = &m_createInfo;
}

PipelineConstruction::DepthStencilState::DepthStencilState(bool enabled)
{
	if (enabled)
	{
		m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		m_createInfo.depthTestEnable = VK_TRUE;
		m_createInfo.depthWriteEnable = VK_TRUE;
		m_createInfo.depthCompareOp = VK_COMPARE_OP_LESS;

		m_createInfo.depthBoundsTestEnable = VK_FALSE;
		m_createInfo.minDepthBounds = 0.0f;
		m_createInfo.maxDepthBounds = 1.0f;

		m_createInfo.stencilTestEnable = VK_FALSE;
		m_createInfo.front = {};
		m_createInfo.back = {};
	}
}
bool PipelineConstruction::DepthStencilState::isValid() const { return m_createInfo.sType == VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; }
void PipelineConstruction::DepthStencilState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pDepthStencilState = &m_createInfo;
}

PipelineConstruction::ViewportState::ViewportState(VkExtent2D extent)
{
	vkinit::Commands::initViewportAndScissor(m_viewport, m_scissor, extent);

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_createInfo.viewportCount = 1;
	m_createInfo.pViewports = &m_viewport;
	m_createInfo.scissorCount = 1;
	m_createInfo.pScissors = &m_scissor;
}
bool PipelineConstruction::ViewportState::isValid() const { return true; }
void PipelineConstruction::ViewportState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pViewportState = &m_createInfo;
}

PipelineConstruction::DynamicState::DynamicState()
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	m_dynamicStates[0] = VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT;
	m_dynamicStates[1] = VkDynamicState::VK_DYNAMIC_STATE_SCISSOR;

	m_createInfo.dynamicStateCount = 2;
	m_createInfo.pDynamicStates = m_dynamicStates;
}
bool PipelineConstruction::DynamicState::isValid() const { return true; }
void PipelineConstruction::DynamicState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pDynamicState = &m_createInfo;
}

VkFormat pickDataFormat(size_t size)
{
	static const VkFormat formats[4]{
		VkFormat::VK_FORMAT_R32_SFLOAT,
		VkFormat::VK_FORMAT_R32G32_SFLOAT,
		VkFormat::VK_FORMAT_R32G32B32_SFLOAT,
		VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	auto index = size / sizeof(float) - 1;

	assert((size % sizeof(float) == 0) && index >= 0 && index < 4 && "The vertex attribute data type is not supported.");

	return formats[index];
}

PipelineConstruction::VertexInputState::VertexInputState(const MeshDescriptor* meshDescriptor) : m_isValid(true)
{
	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	bindingDescription = {};
	if (meshDescriptor)
	{
		auto descriptorCount = meshDescriptor->descriptorCount;
		// Vertex Attribute Descriptions
		attributeDescriptions.resize(descriptorCount);

		size_t offset = 0;
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			auto size = meshDescriptor->elementByteSizes[i];

			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = pickDataFormat(size);
			attributeDescriptions[i].offset = as_uint32(offset);

			offset += size * std::clamp(meshDescriptor->lengths[i], 0_z, 1_z);
		}

		bindingDescription.binding = 0;
		bindingDescription.stride = as_uint32(offset);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		m_createInfo.vertexBindingDescriptionCount = 1;
		m_createInfo.pVertexBindingDescriptions = &bindingDescription;

		m_createInfo.vertexAttributeDescriptionCount = as_uint32(attributeDescriptions.size());
		m_createInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		m_isValid = VertexBinding::validateAttributeAndBindingDescriptions({ bindingDescription }, attributeDescriptions);
	}
}
bool PipelineConstruction::VertexInputState::isValid() const { return m_isValid; }
void PipelineConstruction::VertexInputState::submit(VkGraphicsPipelineCreateInfo& pipelineCI) const
{
	pipelineCI.pVertexInputState = &m_createInfo;
}

bool PipelineConstruction::createPipeline(VkPipeline& pipelineInstance, const VkPipelineLayout pipelineLayout, const VkDevice device, const VkRenderPass renderPass, VkExtent2D swapchainExtent, const VkShader& shader, const MeshDescriptor* descriptor, FaceCulling faceCullingMode, bool depthStencilAttachement)
{
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};

	auto pipeline = GraphicsPipelineCI(pipelineLayout);
	auto renderPassState = RenderPass(renderPass);

	auto vertStage = ShaderStage(shader.vertShader, ShaderStage::SupportedStages::Vertex);
	auto fragStage = ShaderStage(shader.fragShader, ShaderStage::SupportedStages::Fragment);
	PipelineStageCollection allStages(&vertStage, &fragStage);

	auto vertexInputState = VertexInputState(descriptor);
	auto inputAssembly = InputAssembly();
	auto viewportState = ViewportState(swapchainExtent);
	auto rasterizationState = RasterizationState(faceCullingMode);
	auto multisampleState = MultisampleState();
	auto depthStencilState = DepthStencilState(depthStencilAttachement);
	auto colorBlendState = ColorBlendState();

	auto dynamicState = DynamicState();

	ComponentCreateInfoAbstract* allComponents[]{
		&pipeline,
		&renderPassState,

		// Programmable shader stages
		&allStages,

		// Fixed function stages
		&vertexInputState,
		&inputAssembly,
		&viewportState,
		&rasterizationState,
		&multisampleState,
		&depthStencilState,
		&colorBlendState,

		&dynamicState
	};

	for (auto& createInfo : allComponents)
	{
		createInfo->submitIfValid(pipelineCreateInfo);
	}

	return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineInstance) == VK_SUCCESS;
}
