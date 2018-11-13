#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform sampler2D albedoSampler;
layout (set = 0, binding = 1) uniform sampler2D positionSampler;
layout (set = 0, binding = 2) uniform sampler2D normalSampler;
layout (set = 0, binding = 3) uniform sampler2D specSampler;
layout (set = 0, binding = 4) uniform WorldCameraPos{ vec4 camPos; vec4 camFront; vec4 size; vec4 dummy1; mat4 projection; mat4 view; } ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

vec3 ScreenSpaceReflections(vec3 position, vec3 normal);

void main()
{
	vec3 position = texture(positionSampler, inUV).xyz;
	vec3 normal = texture(normalSampler, inUV).xyz;
	float specular = texture(specSampler, inUV).r;

	// The swapchain image is loaded and not cleared in this render pass
	outColor = vec4(ScreenSpaceReflections(position, normalize(normal)), 0.5) * specular;
}

// Screen space reflections
vec3 ScreenSpaceReflections(vec3 position, vec3 normal)
{
	vec3 reflectedColor = vec3(0.0);
	vec3 viewPos = position - ubo.camPos.xyz;
	vec3 reflection = reflect(viewPos, normal);

	float VdotR = max(dot(normalize(viewPos), normalize(reflection)), 0.0);
	float fresnel = pow(VdotR, 5);

	vec3 step = reflection;
	vec3 newPosition = position + step;

	for(int i=0; i<30; i++)
	{
		vec4 newViewPos = ubo.view * vec4(newPosition, 1.0);
		if( newViewPos.z < 0.0 )
			return vec3(0.0, 0.0, 0.0);
		vec4 samplePosition = ubo.projection * newViewPos;
		samplePosition.xy = samplePosition.xy / samplePosition.w * 0.5 + 0.5;

		if(	samplePosition.x < 0.0 || samplePosition.x > 1.0 ||
			samplePosition.y < 0.0 || samplePosition.y > 1.0 ) {
			step *= 0.5;
			newPosition -= step;
			continue;
		}

		float currentDepth = newViewPos.z;
		float sampledDepth = texture(positionSampler, samplePosition.xy).w;

		float delta = abs(currentDepth - sampledDepth);
		if(delta < 0.001f)
		{
			vec2 fadeOnEdges = samplePosition.xy * 2.0 - 1.0;
			fadeOnEdges = abs(fadeOnEdges);
			float fadeAmount = min(1.0 - fadeOnEdges.x, 1.0 - fadeOnEdges.y);

			reflectedColor = texture(albedoSampler, samplePosition.xy).xyz * fresnel * fadeAmount;
			break;
		}
		if(currentDepth > sampledDepth)
		{
			step *= 0.5;
			newPosition -= step;
		}
		else if(currentDepth < sampledDepth)
		{
			newPosition += step;
		}
	}
	return reflectedColor;
}