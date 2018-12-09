#include "Model.h"
#include "../../include/assimp/Importer.hpp"      // C++ importer interface
#include "../../include/assimp/scene.h"           // Output data structure
#include "../../include/assimp/postprocess.h"     // Post processing flags
#include "../../include/assimp/DefaultLogger.hpp"

using namespace vm;

vk::DescriptorSetLayout Model::descriptorSetLayout = nullptr;

mat4 aiMatrix4x4ToMat4(const aiMatrix4x4& m)
{
	return transpose(mat4((float*)&m));
}

void getAllNodes(aiNode* root, std::vector<aiNode*>& allNodes)
{
	for (uint32_t i = 0; i < root->mNumChildren; i++)
		getAllNodes(root->mChildren[i], allNodes);
	if (root) allNodes.push_back(root);
}

mat4 getTranform(aiNode& node)
{
	mat4 transform = aiMatrix4x4ToMat4(node.mTransformation);
	aiNode* tranformNode = &node;
	while (tranformNode->mParent) {
		transform = aiMatrix4x4ToMat4(tranformNode->mParent->mTransformation) * transform;
		tranformNode = tranformNode->mParent;
	}
	return transform;
}

void Model::loadModel(const std::string folderPath, const std::string modelName, bool show)
{
	// Materials, Vertices and Indices load
	Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;
	// Create a logger instance for Console Output
	Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_STDOUT);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(folderPath + modelName,
		//aiProcess_MakeLeftHanded |
		aiProcess_FlipUVs |
		//aiProcess_FlipWindingOrder |
		//aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_CalcTangentSpace |
		//aiProcess_ImproveCacheLocality |
		//aiProcess_OptimizeMeshes |
		//aiProcess_OptimizeGraph |
		0
	);
	if (!scene) exit(-100);

	std::vector<aiNode*> allNodes{};
	getAllNodes(scene->mRootNode, allNodes);

	// set texture types to request
	std::vector<std::tuple<aiTextureType, Mesh::TextureType>> textureMaps
	{
		{aiTextureType_SHININESS, Mesh::RoughnessMap},
		{aiTextureType_AMBIENT, Mesh::MetallicMap},
		{aiTextureType_DIFFUSE, Mesh::DiffuseMap},
		{aiTextureType_NORMALS, Mesh::NormalMap},
		{aiTextureType_SPECULAR, Mesh::SpecularMap},
		{aiTextureType_OPACITY, Mesh::AlphaMap}
	};

	std::vector<Mesh> f_meshes;
	for (unsigned int n = 0; n < allNodes.size(); n++) {
		aiNode& node = *allNodes[n];
		mat4 transform = getTranform(node);
		for (unsigned int i = 0; i < node.mNumMeshes; i++) {
			Mesh myMesh;

			const aiMesh& mesh = *scene->mMeshes[node.mMeshes[i]];
			const aiMaterial& material = *scene->mMaterials[mesh.mMaterialIndex];

			aiColor3D aiAmbient(0.f, 0.f, 0.f);
			material.Get(AI_MATKEY_COLOR_AMBIENT, aiAmbient);
			myMesh.colorEffects.ambient = { aiAmbient.r, aiAmbient.g, aiAmbient.b, 0.f };

			aiColor3D aiDiffuse(1.f, 1.f, 1.f);
			material.Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse);
			float aiOpacity = 1.f;
			material.Get(AI_MATKEY_OPACITY, aiOpacity);
			myMesh.colorEffects.diffuse = { aiDiffuse.r, aiDiffuse.g, aiDiffuse.b, aiOpacity };

			aiColor3D aiSpecular(0.f, 0.f, 0.f);
			material.Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular);
			myMesh.colorEffects.specular = { aiSpecular.r, aiSpecular.g, aiSpecular.b, 100.f };

			// texture maps loading
			for (auto& tm : textureMaps) {
				aiString aiTexPath;
				material.GetTexture(std::get<0>(tm), 0, &aiTexPath);
				myMesh.loadTexture(std::get<1>(tm), folderPath, aiTexPath.C_Str());
			}

			for (unsigned int j = 0; j < mesh.mNumVertices; j++) {
				const aiVector3D& pos = mesh.HasPositions() ? mesh.mVertices[j] : aiVector3D(0.f, 0.f, 0.f);
				const aiVector3D& norm = mesh.HasNormals() ? mesh.mNormals[j] : aiVector3D(0.f, 0.f, 0.f);
				const aiVector3D& uv = mesh.HasTextureCoords(0) ? mesh.mTextureCoords[0][j] : aiVector3D(0.f, 0.f, 0.f);
				const aiVector3D& tangent = mesh.HasTangentsAndBitangents() ? mesh.mTangents[j] : aiVector3D(0.f, 0.f, 0.f);
				const aiVector3D& bitangent = mesh.HasTangentsAndBitangents() ? mesh.mBitangents[j] : aiVector3D(0.f, 0.f, 0.f);
				const aiColor4D& color = mesh.HasVertexColors(0) ? mesh.mColors[0][j] : aiColor4D(1.f, 1.f, 1.f, 1.f);

				vec4 p = transform * vec4(pos.x, pos.y, pos.z, 1.f);
				vec4 n = transform * vec4(norm.x, norm.y, norm.z, 1.f);
				vec4 t = transform * vec4(tangent.x, tangent.y, tangent.z, 1.f);
				vec4 b = transform * vec4(bitangent.x, bitangent.y, bitangent.z, 1.f);
				Vertex v(
					vec3(p),
					vec2((float*)&uv),
					vec3(n),
					vec3(t),
					vec3(b),
					vec4((float*)&color)
				);
				myMesh.vertices.push_back(v);
			}
			for (unsigned int i = 0; i < mesh.mNumFaces; i++) {
				const aiFace& Face = mesh.mFaces[i];
				assert(Face.mNumIndices == 3);
				myMesh.indices.push_back(Face.mIndices[0]);
				myMesh.indices.push_back(Face.mIndices[1]);
				myMesh.indices.push_back(Face.mIndices[2]);
			}
			myMesh.calculateBoundingSphere();
			f_meshes.push_back(myMesh);
		}
	}
	std::sort(f_meshes.begin(), f_meshes.end(), [](Mesh& a, Mesh& b) -> bool { return a.hasAlpha < b.hasAlpha; });
	for (auto &m : f_meshes) {
		m.vertexOffset = numberOfVertices;
		m.indexOffset = numberOfIndices;

		meshes.push_back(m);
		numberOfVertices += static_cast<uint32_t>(m.vertices.size());
		numberOfIndices += static_cast<uint32_t>(m.indices.size());
	}

	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
	createDescriptorSets();
	name = modelName;
	render = show;

	// resizing the model to always be at a certain magnitude
	float factor = 20.0f / getBoundingSphere().w;
	transform = scale(transform, vec3(factor, factor, factor));
	for (auto &m : f_meshes) {
		m.transform = transform;
	}
}

void Model::draw(Pipeline& pipeline, vk::CommandBuffer& cmd, const uint32_t& modelID, bool deferredRenderer, Shadows* shadows, vk::DescriptorSet* DSLights)
{
	if (render)
	{
		const vk::DeviceSize offset{ 0 };
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);

		cmd.bindVertexBuffers(0, 1, &vertexBuffer.buffer, &offset);
		cmd.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint32);

		for (auto& mesh : meshes) {
			if (mesh.render && !mesh.cull) {
				if (deferredRenderer) {
					cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeinfo.layout, 0, { descriptorSet, mesh.descriptorSet }, nullptr);
				}
				else {
					const uint32_t dOffsets = modelID * sizeof(ShadowsUBO);
					cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeinfo.layout, 0, { shadows->descriptorSet, mesh.descriptorSet, descriptorSet, *DSLights }, dOffsets);
				}
				cmd.drawIndexed(static_cast<uint32_t>(mesh.indices.size()), 1, mesh.indexOffset, mesh.vertexOffset, 0);
			}
		}
	}
}

// position x, y, z and radius w
vec4 Model::getBoundingSphere()
{
	for (auto &mesh : meshes) {
		float temp = mesh.boundingSphere.w + length(vec3(mesh.boundingSphere.x, mesh.boundingSphere.x, mesh.boundingSphere.z));
		if (temp > boundingSphere.w)
			boundingSphere.w = temp;
	}
	return boundingSphere;
}

vk::DescriptorSetLayout Model::getDescriptorSetLayout(vk::Device device)
{
	if (!descriptorSetLayout) {
		std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBinding{};

		// binding for model mvp matrix
		descriptorSetLayoutBinding.push_back(vk::DescriptorSetLayoutBinding()
			.setBinding(0) // binding number in shader stages
			.setDescriptorCount(1) // number of descriptors contained
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex)); // which pipeline shader stages can access

		auto const createInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount((uint32_t)descriptorSetLayoutBinding.size())
			.setPBindings(descriptorSetLayoutBinding.data());
		descriptorSetLayout = device.createDescriptorSetLayout(createInfo);
	}
	return descriptorSetLayout;
}

void Model::createVertexBuffer()
{
	std::vector<Vertex> vertices{};
	for (auto& mesh : meshes) {
		for (auto& vertex : mesh.vertices)
			vertices.push_back(vertex);
	}

	vertexBuffer.createBuffer(sizeof(Vertex)*vertices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// Staging buffer
	Buffer staging;
	staging.createBuffer(sizeof(Vertex)*vertices.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	staging.data = vulkan->device.mapMemory(staging.memory, 0, staging.size);
	memcpy(staging.data, vertices.data(), sizeof(Vertex)*vertices.size());
	vulkan->device.unmapMemory(staging.memory);

	vertexBuffer.copyBuffer(staging.buffer, staging.size);
	staging.destroy();
}

void Model::createIndexBuffer()
{
	std::vector<uint32_t> indices{};
	for (auto& mesh : meshes) {
		for (auto& index : mesh.indices)
			indices.push_back(index);
	}
	indexBuffer.createBuffer(sizeof(uint32_t)*indices.size(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

	// Staging buffer
	Buffer staging;
	staging.createBuffer(sizeof(uint32_t)*indices.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	staging.data = vulkan->device.mapMemory(staging.memory, 0, staging.size);
	memcpy(staging.data, indices.data(), sizeof(uint32_t)*indices.size());
	vulkan->device.unmapMemory(staging.memory);

	indexBuffer.copyBuffer(staging.buffer, staging.size);
	staging.destroy();
}

void Model::createUniformBuffers()
{
	// since the uniform buffers are unique for each model, they are not bigger than 256 in size
	uniformBuffer.createBuffer(256, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	uniformBuffer.data = vulkan->device.mapMemory(uniformBuffer.memory, 0, uniformBuffer.size);
}

void Model::createDescriptorSets()
{
	auto const allocateInfo = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(vulkan->descriptorPool)
		.setDescriptorSetCount(1)
		.setPSetLayouts(&descriptorSetLayout);
	descriptorSet = vulkan->device.allocateDescriptorSets(allocateInfo)[0];

	// Model MVP
	auto const mvpWriteSet = vk::WriteDescriptorSet()
		.setDstSet(descriptorSet)								// DescriptorSet dstSet;
		.setDstBinding(0)										// uint32_t dstBinding;
		.setDstArrayElement(0)									// uint32_t dstArrayElement;
		.setDescriptorCount(1)									// uint32_t descriptorCount;
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)	// DescriptorType descriptorType;
		.setPBufferInfo(&vk::DescriptorBufferInfo()				// const DescriptorBufferInfo* pBufferInfo;
			.setBuffer(uniformBuffer.buffer)						// Buffer buffer;
			.setOffset(0)											// DeviceSize offset;
			.setRange(uniformBuffer.size));							// DeviceSize range;

	vulkan->device.updateDescriptorSets(mvpWriteSet, nullptr);

	for (auto& mesh : meshes) {
		auto const allocateInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(vulkan->descriptorPool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&mesh.descriptorSetLayout);
		mesh.descriptorSet = vulkan->device.allocateDescriptorSets(allocateInfo)[0];

		// Texture
		std::vector<vk::WriteDescriptorSet> textureWriteSets(6);

		textureWriteSets[0] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(0)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.texture.sampler)								// Sampler sampler;
				.setImageView(mesh.texture.view)								// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		textureWriteSets[1] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(1)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.normalsTexture.sampler)						// Sampler sampler;
				.setImageView(mesh.normalsTexture.view)							// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		textureWriteSets[2] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(2)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.specularTexture.sampler)						// Sampler sampler;
				.setImageView(mesh.specularTexture.view)						// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		textureWriteSets[3] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(3)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.alphaTexture.sampler)							// Sampler sampler;
				.setImageView(mesh.alphaTexture.view)							// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		textureWriteSets[4] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(4)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.roughnessTexture.sampler)						// Sampler sampler;
				.setImageView(mesh.roughnessTexture.view)						// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		textureWriteSets[5] = vk::WriteDescriptorSet()
			.setDstSet(mesh.descriptorSet)									// DescriptorSet dstSet;
			.setDstBinding(5)												// uint32_t dstBinding;
			.setDstArrayElement(0)											// uint32_t dstArrayElement;
			.setDescriptorCount(1)											// uint32_t descriptorCount;
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)	// DescriptorType descriptorType;
			.setPImageInfo(&vk::DescriptorImageInfo()						// const DescriptorImageInfo* pImageInfo;
				.setSampler(mesh.metallicTexture.sampler)						// Sampler sampler;
				.setImageView(mesh.metallicTexture.view)						// ImageView imageView;
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));		// ImageLayout imageLayout;

		vulkan->device.updateDescriptorSets(textureWriteSets, nullptr);
	}
}

void Model::destroy()
{
	for (auto& mesh : meshes) {
		mesh.vertices.clear();
		mesh.vertices.shrink_to_fit();
		mesh.indices.clear();
		mesh.indices.shrink_to_fit();
	}

	for (auto& texture : Mesh::uniqueTextures)
		texture.second.destroy();
	Mesh::uniqueTextures.clear();

	vertexBuffer.destroy();
	indexBuffer.destroy();
	uniformBuffer.destroy();

	if (Model::descriptorSetLayout) {
		vulkan->device.destroyDescriptorSetLayout(Model::descriptorSetLayout);
		Model::descriptorSetLayout = nullptr;
	}

	if (Mesh::descriptorSetLayout) {
		vulkan->device.destroyDescriptorSetLayout(Mesh::descriptorSetLayout);
		Mesh::descriptorSetLayout = nullptr;
	}
}