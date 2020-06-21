#include "Pipeline.h"
#include "../Shader/Shader.h"
#include "../VulkanContext/VulkanContext.h"
#include <vulkan/vulkan.hpp>

namespace vm
{
	PipelineCreateInfo::PipelineCreateInfo()
	{
		pVertShader = nullptr;
		pFragShader = nullptr;
		vertexInputBindingDescriptions = std::vector<vk::VertexInputBindingDescription>();
		vertexInputAttributeDescriptions = std::vector<vk::VertexInputAttributeDescription>();
		width = 0.f;
		height = 0.f;
		pushConstantStage = PushConstantStage::Vertex;
		pushConstantSize = 0;
		cullMode = CullMode::None;
		colorBlendAttachments = std::vector<vk::PipelineColorBlendAttachmentState>();
		dynamicStates = std::vector<vk::DynamicState>();
		descriptorSetLayouts = std::vector<vk::DescriptorSetLayout>();
	}

	PipelineCreateInfo::~PipelineCreateInfo()
	{
	}

	Pipeline::Pipeline()
	{
		pipeline = vk::Pipeline();
		pipelineLayout = vk::PipelineLayout();
		compinfo = vk::ComputePipelineCreateInfo();
	}

	Pipeline::~Pipeline()
	{
	}

	void Pipeline::createGraphicsPipeline()
	{
		vk::GraphicsPipelineCreateInfo pipeinfo;

		vk::ShaderModuleCreateInfo vsmci;
		vsmci.codeSize = info.pVertShader->byte_size();
		vsmci.pCode = info.pVertShader->get_spriv();
		vk::ShaderModule vertModule = VulkanContext::get()->device->createShaderModule(vsmci);

		vk::PipelineShaderStageCreateInfo pssci1;
		pssci1.stage = vk::ShaderStageFlagBits::eVertex;
		pssci1.module = vertModule;
		pssci1.pName = "main";

		vk::ShaderModuleCreateInfo fsmci;
		vk::PipelineShaderStageCreateInfo pssci2;
		vk::ShaderModule fragModule;
		if (info.pFragShader)
		{
			fsmci.codeSize = info.pFragShader->byte_size();
			fsmci.pCode = info.pFragShader->get_spriv();

			pssci2.stage = vk::ShaderStageFlagBits::eFragment;
			pssci2.module = VulkanContext::get()->device->createShaderModule(fsmci);;
			pssci2.pName = "main";
		}

		std::vector<vk::PipelineShaderStageCreateInfo> stages{ pssci1 };
		if (info.pFragShader)
			stages.push_back(pssci2);

		pipeinfo.stageCount = static_cast<uint32_t>(stages.size());
		pipeinfo.pStages = stages.data();

		// Vertex Input state
		vk::PipelineVertexInputStateCreateInfo pvisci;
		pvisci.vertexBindingDescriptionCount = static_cast<uint32_t>(info.vertexInputBindingDescriptions->size());
		pvisci.vertexAttributeDescriptionCount = static_cast<uint32_t>(info.vertexInputAttributeDescriptions->size());
		pvisci.pVertexBindingDescriptions = info.vertexInputBindingDescriptions->data();
		pvisci.pVertexAttributeDescriptions = info.vertexInputAttributeDescriptions->data();
		pipeinfo.pVertexInputState = &pvisci;

		// Input Assembly stage
		vk::PipelineInputAssemblyStateCreateInfo piasci;
		piasci.topology = vk::PrimitiveTopology::eTriangleList;
		piasci.primitiveRestartEnable = VK_FALSE;
		pipeinfo.pInputAssemblyState = &piasci;

		// Viewports and Scissors
		vk::Viewport vp;
		vp.x = 0.0f;
		vp.y = 0.0f;
		vp.width = info.width;
		vp.height = info.height;
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;

		vk::Rect2D r2d;
		r2d.extent = vk::Extent2D{ static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height) };

		vk::PipelineViewportStateCreateInfo pvsci;
		pvsci.viewportCount = 1;
		pvsci.pViewports = &vp;
		pvsci.scissorCount = 1;
		pvsci.pScissors = &r2d;
		pipeinfo.pViewportState = &pvsci;

		// Rasterization state
		vk::PipelineRasterizationStateCreateInfo prsci;
		prsci.depthClampEnable = VK_FALSE;
		prsci.rasterizerDiscardEnable = VK_FALSE;
		prsci.polygonMode = vk::PolygonMode::eFill;
		prsci.cullMode = static_cast<vk::CullModeFlagBits>(info.cullMode);
		prsci.frontFace = vk::FrontFace::eClockwise;
		prsci.depthBiasEnable = VK_FALSE;
		prsci.depthBiasConstantFactor = 0.0f;
		prsci.depthBiasClamp = 0.0f;
		prsci.depthBiasSlopeFactor = 0.0f;
		prsci.lineWidth = 1.0f;
		pipeinfo.pRasterizationState = &prsci;

		// Multisample state
		vk::PipelineMultisampleStateCreateInfo pmsci;
		pmsci.rasterizationSamples = vk::SampleCountFlagBits::e1;
		pmsci.sampleShadingEnable = VK_FALSE;
		pmsci.minSampleShading = 1.0f;
		pmsci.pSampleMask = nullptr;
		pmsci.alphaToCoverageEnable = VK_FALSE;
		pmsci.alphaToOneEnable = VK_FALSE;
		pipeinfo.pMultisampleState = &pmsci;

		// Depth stencil state
		vk::PipelineDepthStencilStateCreateInfo pdssci;
		pdssci.depthTestEnable = VK_TRUE;
		pdssci.depthWriteEnable = VK_TRUE;
		pdssci.depthCompareOp = vk::CompareOp::eGreater;
		pdssci.depthBoundsTestEnable = VK_FALSE;
		pdssci.stencilTestEnable = VK_FALSE;
		pdssci.front.compareOp = vk::CompareOp::eAlways;
		pdssci.back.compareOp = vk::CompareOp::eAlways;
		pdssci.minDepthBounds = 0.0f;
		pdssci.maxDepthBounds = 0.0f;
		pipeinfo.pDepthStencilState = &pdssci;

		// Color Blending state
		vk::PipelineColorBlendStateCreateInfo pcbsci;
		pcbsci.logicOpEnable = VK_FALSE;
		pcbsci.logicOp = vk::LogicOp::eAnd;
		pcbsci.attachmentCount = static_cast<uint32_t>(info.colorBlendAttachments->size());
		pcbsci.pAttachments = info.colorBlendAttachments->data();
		float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		memcpy(pcbsci.blendConstants, blendConstants, 4 * sizeof(float));
		pipeinfo.pColorBlendState = &pcbsci;

		// Dynamic state
		vk::PipelineDynamicStateCreateInfo dsi;
		dsi.dynamicStateCount = static_cast<uint32_t>(info.dynamicStates->size());
		dsi.pDynamicStates = info.dynamicStates->data();
		pipeinfo.pDynamicState = &dsi;

		// Push Constant Range
		vk::PushConstantRange pcr;
		pcr.stageFlags = static_cast<vk::ShaderStageFlagBits>(info.pushConstantStage);
		pcr.size = info.pushConstantSize;

		// Pipeline Layout
		vk::PipelineLayoutCreateInfo plci;
		plci.setLayoutCount = static_cast<uint32_t>(info.descriptorSetLayouts->size());
		plci.pSetLayouts = info.descriptorSetLayouts->data();
		plci.pushConstantRangeCount = 1;
		plci.pPushConstantRanges = info.pushConstantSize ? &pcr : nullptr;
		pipelineLayout = VulkanContext::get()->device->createPipelineLayout(plci);
		pipeinfo.layout = pipelineLayout.Value();

		// Render Pass
		pipeinfo.renderPass = info.renderPass.Value();

		// Subpass (Index of subpass this pipeline will be used in)
		pipeinfo.subpass = 0;

		// Base Pipeline Handle
		pipeinfo.basePipelineHandle = nullptr;

		// Base Pipeline Index
		pipeinfo.basePipelineIndex = -1;

		pipeline = VulkanContext::get()->device->createGraphicsPipelines(nullptr, pipeinfo).at(0);

		// destroy Shader Modules
		VulkanContext::get()->device->destroyShaderModule(vertModule);
		if (fragModule)
			VulkanContext::get()->device->destroyShaderModule(fragModule);
	}

	void Pipeline::destroy()
	{
		if (pipelineLayout && pipelineLayout.Value()) {
			VulkanContext::get()->device->destroyPipelineLayout(pipelineLayout.Value());
			*pipelineLayout = nullptr;
		}

		if (pipeline && pipeline.Value()) {
			VulkanContext::get()->device->destroyPipeline(pipeline.Value());
			*pipeline = nullptr;
		}
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutComposition()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(3, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(4, vk::DescriptorType::eUniformBuffer),
				layoutBinding(5, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(6, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(7, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(8, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(9, vk::DescriptorType::eUniformBuffer)
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutBrightFilter()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutGaussianBlurH()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutGaussianBlurV()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutCombine()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutDOF()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutFXAA()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutMotionBlur()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(3, vk::DescriptorType::eUniformBuffer),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutSSAO()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(3, vk::DescriptorType::eUniformBuffer),
				layoutBinding(4, vk::DescriptorType::eUniformBuffer),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutSSAOBlur()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutSSR()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(3, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(4, vk::DescriptorType::eUniformBuffer),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutTAA()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(3, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(4, vk::DescriptorType::eUniformBuffer)
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutTAASharpen()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eFragment, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler),
				layoutBinding(1, vk::DescriptorType::eUniformBuffer)
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutShadows()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;
		
		if (!DSLayout)
		{
			const auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType, const vk::ShaderStageFlags& stageFlags) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, stageFlags, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler,vk::ShaderStageFlagBits::eFragment),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutMesh()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto const layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, vk::ShaderStageFlagBits::eVertex, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eUniformBuffer),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutPrimitive()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			auto const layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType, const vk::ShaderStageFlags& stageFlag) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, stageFlag, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(1, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(2, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(3, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(4, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment),
				layoutBinding(5, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex),
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutModel()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			vk::DescriptorSetLayoutBinding dslb;
			dslb.binding = 0;
			dslb.descriptorCount = 1; // number of descriptors contained
			dslb.descriptorType = vk::DescriptorType::eUniformBuffer;
			dslb.stageFlags = vk::ShaderStageFlagBits::eVertex;

			vk::DescriptorSetLayoutCreateInfo dslci;
			dslci.bindingCount = 1;
			dslci.pBindings = &dslb;
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(dslci);
		}

		return DSLayout;
	}

	vk::DescriptorSetLayout& Pipeline::getDescriptorSetLayoutSkybox()
	{
		static vk::DescriptorSetLayout DSLayout = nullptr;

		if (!DSLayout)
		{
			const auto layoutBinding = [](uint32_t binding, vk::DescriptorType descriptorType, const vk::ShaderStageFlags& stageFlags) {
				return vk::DescriptorSetLayoutBinding{ binding, descriptorType, 1, stageFlags, nullptr };
			};
			std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings{
				layoutBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
			};
			vk::DescriptorSetLayoutCreateInfo descriptorLayout;
			descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayout.pBindings = setLayoutBindings.data();
			DSLayout = VulkanContext::get()->device->createDescriptorSetLayout(descriptorLayout);
		}
		return DSLayout;
	}
}