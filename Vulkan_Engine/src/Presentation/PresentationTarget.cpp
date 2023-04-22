#include "pch.h"
#include "Texture.h"
#include "PresentationTarget.h"
#include "Presentation/Device.h"
#include "Presentation/HardwareDevice.h"

#include "Engine/Window.h"
#include "VkTypes/VkTexture.h"
#include "PipelineBinding.h"

#include "ShaderSource.h"
#include "VkTypes/VkShader.h"
#include "VkTypes/VkGraphicsPipeline.h"
#include "VkTypes/PipelineConstructor.h"
#include "Mesh.h"
#include "DescriptorPoolManager.h"

#include "Passes/ShadowmapPass.h"
#include "Passes/DebugPass.h"

namespace Presentation
{
	PresentationTarget::PresentationTarget(const HardwareDevice& presentationHardware, const Device& presentationDevice, Window const* wnd, bool depthAttachment, uint32_t swapchainCount)
		: m_window(wnd), m_hasDepthAttachment(depthAttachment)
	{
		m_isInitialized = createPresentationTarget(presentationHardware, presentationDevice, swapchainCount) &&
			tryInitialize(m_globalPipelineState, presentationDevice.getDevice());
		
		// Initialize default shaders
		VkShader::ensureDefaultShader(presentationDevice.getDevice());
		
		m_emptyShadowMap = MAKEUNQ<EmptyShadowMap>(*this, presentationDevice, VkShader::findShader(0u));

		// Creates the pipeline, but doesn't manage its lifetime
		m_shadowMapModule = MAKEUNQ<ShadowMap>(*this, presentationDevice.getDevice(), m_globalPipelineState->getDepthOnlyPipelineLayout(), true, 2048u);

		const auto* debugQuadShader = VkShader::findShader(2u);
		// Creates the pipeline, but doesn't manage its lifetime
		m_debugModule = MAKEUNQ<DebugPass>(*this, presentationDevice.getDevice(), debugQuadShader, m_globalPipelineState->getForwardPipelineLayout(), 
			getRenderPass(), getSwapchainExtent(), m_shadowMapModule->getTexture2D());
	}

	bool PresentationTarget::createPresentationTarget(const HardwareDevice& presentationHardware, const Device& presentationDevice, uint32_t swapchainCount)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(presentationHardware.getActiveGPU(), presentationDevice.getSurface(), &m_capabilities);
		m_swapChainExtent = chooseSwapExtent(m_window->get());

		auto vkdevice = presentationDevice.getDevice();

		return createSwapChain(swapchainCount, presentationHardware, presentationDevice, m_hasDepthAttachment) &&
			createSwapChainImageViews(vkdevice) &&
			createRenderPass(vkdevice) &&
			createFramebuffers(vkdevice) &&
			transitionSwapchainLayout(presentationDevice);
	}
	bool PresentationTarget::hasDepthAttachement() { return m_depthImage ? true : false; }

	VkExtent2D PresentationTarget::chooseSwapExtent(const SDL_Window* window)
	{
		if (m_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return m_capabilities.currentExtent;
		}
		else
		{
			int width, height;
			SDL_Vulkan_GetDrawableSize(const_cast<SDL_Window*>(window), &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, m_capabilities.minImageExtent.width, m_capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, m_capabilities.minImageExtent.height, m_capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	bool PresentationTarget::createSwapChain(uint32_t imageCount, const HardwareDevice& hardware, const Device& device, bool createDepthAttachement)
	{
		auto surfaceFormat = hardware.chooseSwapSurfaceFormat();
		auto presentationMode = hardware.chooseSwapPresentMode();
		auto extent = chooseSwapExtent(m_window->get());

		imageCount = std::max(imageCount, m_capabilities.minImageCount);
		if (m_capabilities.maxImageCount != 0)
			imageCount = std::min(imageCount, m_capabilities.maxImageCount);

		bool isSuccess = vkinit::Texture::createSwapchain(m_swapchain, device.getDevice(), device.getSurface(),
			imageCount, extent, presentationMode, surfaceFormat, m_capabilities.currentTransform);

		if (createDepthAttachement)
		{
			auto maci = VkMemoryAllocator::getInstance()->createAllocationDescriptor(VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			m_depthImage = MAKEUNQ<VkTexture>(device.getDevice(), maci, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, extent);
		}
		else m_depthImage = nullptr;

		if (isSuccess)
		{
			m_swapChainExtent = extent;
			m_swapChainImageFormat = surfaceFormat.format;

			vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, nullptr);
			m_swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(device.getDevice(), m_swapchain, &imageCount, m_swapChainImages.data());

		}

		return isSuccess;
	}
	
	bool PresentationTarget::transitionSwapchainLayout(const Device& device)
	{
		return device.submitImmediatelyAndWaitCompletion([=](VkCommandBuffer cmd)
		{
			for (auto image : m_swapChainImages)
				Texture::transitionImageLayout(cmd, image, m_swapChainImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1u);
		});
	}

	bool PresentationTarget::createRenderPass(VkDevice device)
	{
		return vkinit::Surface::createRenderPass(m_renderPass, device, m_swapChainImageFormat, hasDepthAttachement());
	}

	bool PresentationTarget::createSwapChainImageViews(VkDevice device)
	{
		auto imageCount = m_swapChainImages.size();
		m_swapChainImageViews.resize(imageCount);

		for (int i = 0; i < imageCount; i++)
		{
			if (!vkinit::Texture::createTextureImageView(m_swapChainImageViews[i], device, m_swapChainImages[i], VK_FORMAT_B8G8R8A8_SRGB, 1u))
				return false;
		}

		return true;
	}

	bool PresentationTarget::createFramebuffers(VkDevice device)
	{
		auto imageViewSize = m_swapChainImageViews.size();
		m_swapChainFrameBuffers.resize(imageViewSize);

		std::array<VkImageView, 2> attachments;
		auto attachmentCount = 1;

		if (hasDepthAttachement())
		{
			attachments[1] = m_depthImage->imageView;
			attachmentCount += 1;
		}

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = attachmentCount;
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		for (size_t i = 0; i < imageViewSize; i++)
		{
			attachments[0] = m_swapChainImageViews[i];

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS)
				return false;
		}

		return true;
	}

	void PresentationTarget::releaseSwapChain(VkDevice device)
	{
		if (hasDepthAttachement())
		{
			m_depthImage->release(device);
		}

		int count = static_cast<uint32_t>(m_swapChainImageViews.size());
		assert(m_swapChainImageViews.size() == m_swapChainFrameBuffers.size() && "Frame buffer count should be equal to image view count.");
		for (int i = 0; i < count; i++)
		{
			vkDestroyFramebuffer(device, m_swapChainFrameBuffers[i], nullptr);
			vkDestroyImageView(device, m_swapChainImageViews[i], nullptr);

			m_swapChainFrameBuffers[i] = nullptr;
			m_swapChainImageViews[i] = nullptr;
		}

		vkDestroySwapchainKHR(device, m_swapchain, nullptr);
		vkDestroyRenderPass(device, m_renderPass, nullptr);

		m_renderPass = VK_NULL_HANDLE;
		m_swapchain = VK_NULL_HANDLE;
	}

	void PresentationTarget::releaseAllResources(VkDevice device)
	{
		m_globalPipelineState->release(device);
		m_globalPipelineState = nullptr;

		releaseSwapChain(device);

		{
			m_debugModule->release(device);
			m_debugModule = nullptr;
		}

		m_emptyShadowMap->release(device);
		m_emptyShadowMap = nullptr;

		if (m_shadowMapModule)
		{
			m_shadowMapModule->release(device);
			m_shadowMapModule = nullptr;
		}
	}

	PresentationTarget::~PresentationTarget() = default;
	bool PresentationTarget::isInitialized() const { return m_isInitialized; }
	VkSwapchainKHR PresentationTarget::getSwapchain() const { return m_swapchain; }
	VkExtent2D PresentationTarget::getSwapchainExtent() const { return m_swapChainExtent; }
	VkRenderPass PresentationTarget::getRenderPass() const { return m_renderPass; }
	VkImage PresentationTarget::getSwapchainImage(uint32_t index) const { return m_swapChainImages[index % SWAPCHAIN_IMAGE_COUNT]; }
	VkImageView PresentationTarget::getSwapchainImageView(uint32_t index) const { return m_swapChainImageViews[index % SWAPCHAIN_IMAGE_COUNT]; }
	VkFramebuffer PresentationTarget::getSwapchainFrameBuffers(uint32_t index) const { return m_swapChainFrameBuffers[index % SWAPCHAIN_IMAGE_COUNT]; }
}