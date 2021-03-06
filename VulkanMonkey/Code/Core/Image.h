#pragma once
#include "Base.h"

namespace vk
{
	class Image;
	class DeviceMemory;
	class ImageView;
	class Sampler;
	struct Extent2D;
	enum class Format;
	enum class ImageLayout;
	enum class ImageTiling;
	enum class Filter;
	enum class ImageViewType;
	enum class SamplerAddressMode;
	enum class BorderColor;
	enum class CompareOp;
	enum class SamplerMipmapMode;
	struct PipelineColorBlendAttachmentState;
	class CommandBuffer;
	enum class SampleCountFlagBits;
	class Buffer;

	template<class T1, class T2> class Flags;
	enum class ImageCreateFlagBits;
	enum class PipelineStageFlagBits;
	enum class AccessFlagBits;
	enum class ImageAspectFlagBits;
	enum class ImageUsageFlagBits;
	enum class MemoryPropertyFlagBits;
	using ImageCreateFlags = Flags<ImageCreateFlagBits, uint32_t>;
	using PipelineStageFlags = Flags<PipelineStageFlagBits, uint32_t>;
	using AccessFlags = Flags<AccessFlagBits, uint32_t>;
	using ImageAspectFlags = Flags<ImageAspectFlagBits, uint32_t>;
	using ImageUsageFlags = Flags<ImageUsageFlagBits, uint32_t>;
	using MemoryPropertyFlags = Flags<MemoryPropertyFlagBits, uint32_t>;
	using Bool32 = uint32_t;
}

namespace vm
{
	enum class LayoutState
	{
		ColorRead,
		ColorWrite,
		DepthRead,
		DepthWrite
	};

	class Context;

	class Image
	{
	public:
		Image();
		~Image();
		Ref<vk::Image> image;
		Ref<vk::DeviceMemory> memory;
		Ref<vk::ImageView> view;
		Ref<vk::Sampler> sampler;
		uint32_t width;
		uint32_t height;
		float width_f;
		float height_f;
		Ref<vk::Extent2D> extent;

		// values
		Ref<vk::SampleCountFlagBits> samples;
		LayoutState layoutState;
		Ref<vk::Format> format;
		Ref<vk::ImageLayout> initialLayout;
		Ref<vk::ImageTiling> tiling;
		uint32_t mipLevels;
		uint32_t arrayLayers;
		bool anisotropyEnabled;
		float minLod, maxLod, maxAnisotropy;
		Ref<vk::Filter> filter;
		Ref<vk::ImageCreateFlags> imageCreateFlags;
		Ref<vk::ImageViewType> viewType;
		Ref<vk::SamplerAddressMode> addressMode;
		Ref<vk::BorderColor> borderColor;
		bool samplerCompareEnable;
		Ref<vk::CompareOp> compareOp;
		Ref<vk::SamplerMipmapMode> samplerMipmapMode;
		Ref<vk::PipelineColorBlendAttachmentState> blentAttachment;

		void transitionImageLayout(
			vk::CommandBuffer cmd,
			vk::ImageLayout oldLayout,
			vk::ImageLayout newLayout,
			const vk::PipelineStageFlags& oldStageMask,
			const vk::PipelineStageFlags& newStageMask,
			const vk::AccessFlags& srcMask,
			const vk::AccessFlags& dstMask,
			const vk::ImageAspectFlags& aspectFlags) const;
		void createImage(uint32_t width, uint32_t height, vk::ImageTiling tiling, const vk::ImageUsageFlags& usage, const vk::MemoryPropertyFlags& properties);
		void createImageView(const vk::ImageAspectFlags& aspectFlags);
		void transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
		void changeLayout(const vk::CommandBuffer& cmd, LayoutState state);
		void copyBufferToImage(vk::Buffer buffer, uint32_t baseLayer = 0) const;
		void copyColorAttachment(const vk::CommandBuffer& cmd, Image& renderedImage) const;
		void generateMipMaps() const;
		void createSampler();
		void destroy();
	};
}