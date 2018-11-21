#pragma once
#include "VulkanContext.h"
#include "Image.h"
#include "Buffer.h"
#include "Image.h"
#include "Pipeline.h"
#include "Math.h"

namespace vm {
	struct Shadows
	{
		VulkanContext* vulkan;
		Shadows(VulkanContext* vulkan);

		static bool shadowCast;
		static uint32_t imageSize;
		static vk::DescriptorSetLayout descriptorSetLayout;
		static vk::DescriptorSetLayout getDescriptorSetLayout(vk::Device device);
		vk::RenderPass renderPass;
		vk::RenderPass getRenderPass();
		Image texture = Image(vulkan);
		vk::DescriptorSet descriptorSet;
		std::vector<vk::Framebuffer> frameBuffer;
		Buffer uniformBuffer = Buffer(vulkan);
		Pipeline pipeline = Pipeline(vulkan);

		void createFrameBuffers(uint32_t bufferCount);
		void createDynamicUniformBuffer(size_t num_of_objects);
		void createDescriptorSet();
		void destroy();
	};


	struct ShadowsUBO
	{
		vm::mat4 projection, view, model;
		float castShadows, dummy[15]; // for 256 bytes align
	};
}
