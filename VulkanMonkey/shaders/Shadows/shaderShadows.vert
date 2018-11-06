#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 i_Normal;
layout(location = 2) in vec2 i_TexCoords;
layout(location = 3) in vec4 i_Tangent;

layout( set = 0, binding = 0 ) uniform UniformBuffer {
	mat4 projection;
	mat4 lightView;
	mat4 model;
	mat4 dummy;
}ubo;

void main() {
	gl_Position = ubo.projection * ubo.lightView * ubo.model * vec4(position, 1.0);
}