#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Light {
	vec4 color;
	vec4 position;
	vec4 attenuation; };

vec2 poissonDisk[8] = vec2[](
	vec2(0.493393f, 0.394269f),
	vec2(0.798547f, 0.885922f),
	vec2(0.247322f, 0.92645f),
	vec2(0.0514542f, 0.140782f),
	vec2(0.831843f, 0.00955229f),
	vec2(0.428632f, 0.0171514f),
	vec2(0.015656f, 0.749779f),
	vec2(0.758385f, 0.49617f));

layout (constant_id = 0) const int NUM_LIGHTS = 1;
layout (set = 1, binding = 0) uniform sampler2D tSampler;
layout (set = 1, binding = 1) uniform sampler2D normSampler;
layout (set = 1, binding = 2) uniform sampler2D specSampler;
layout (set = 1, binding = 3) uniform sampler2D alphaSampler;
layout (set = 1, binding = 4) uniform sampler2D roughSampler;
layout (set = 1, binding = 5) uniform sampler2D metalSampler;
layout (set = 0, binding = 1) uniform sampler2DShadow shadowMapSampler;
layout (set = 3, binding = 0) uniform UBO { vec4 camPos; Light lights[NUM_LIGHTS+1]; } ubo;

layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;
layout (location = 5) in vec3 inColor;
layout (location = 6) in float castShadows;
layout (location = 7) in mat4 shadow_coords;

layout(location = 0) out vec4 outColor;

vec3 calculateShadow(int mainLight, vec3 fragPos, vec3 normal, vec3 albedo, float specular);
vec3 calculateColor(int light, vec3 fragPos, vec3 normal, vec3 albedo, float specular);

void main()
{
	float alpha = texture(alphaSampler, inUV).r;
	if (alpha < 0.8)
		discard;

	vec3 T = normalize(inTangent);
	vec3 B = normalize(inBitangent);
	vec3 N = normalize(inNormal);
	mat3 TBN = mat3(T, B, N);

	vec3 fragPos = inFragPos;
	vec3 normal = normalize(TBN * (texture(normSampler, inUV).rgb * 2.0 - 1.0));
	vec3 albedo = texture(tSampler, inUV).rgb;
	float specular = texture(specSampler, inUV).r;
	
	// Ambient
	vec3 fragColor = 0.3 * albedo.rgb;

	fragColor += calculateShadow(0, fragPos, normal, albedo, specular);

	for(int i=1; i<NUM_LIGHTS+1; i++)
		fragColor += calculateColor(i, fragPos, normal, albedo, specular);

	outColor = vec4(fragColor, alpha);
}

vec3 calculateShadow(int mainLight, vec3 fragPos, vec3 normal, vec3 albedo, float specular)
{
	vec4 s_coords =  shadow_coords * vec4(fragPos, 1.0);
	s_coords = s_coords / s_coords.w;
	float shadow = 0.0;
	for (int i = 0; i < 8 * castShadows ; i++)
		shadow += 0.125 * texture( shadowMapSampler, vec3( s_coords.xy + poissonDisk[i]*0.0008, s_coords.z-0.0001 ));

	// Light to fragment
	vec3 L = fragPos - ubo.lights[mainLight].position.xyz;

	// Viewer to fragment
	vec3 V = fragPos - ubo.camPos.xyz;

	// Diffuse part
	vec3 diff = ubo.lights[mainLight].color.rgb * albedo * ubo.lights[mainLight].color.a;

	// Specular part
	L = normalize(L);
	vec3 N = normalize(normal);
	vec3 R = reflect(-L, N);
	V = normalize(V);
	float RdotV = max(0.0, dot(R, V));
	vec3 spec = ubo.lights[mainLight].color.rgb * specular * pow(RdotV, 32.0);

	return shadow * (diff + spec);
}

vec3 calculateColor(int light, vec3 fragPos, vec3 normal, vec3 albedo, float specular)
{
	// Light to fragment
	vec3 L =  fragPos - ubo.lights[light].position.xyz;

	// Distance from light to fragment
	float dist = length(L);
	if (dist > ubo.lights[light].attenuation.x*5.0)
		return vec3(0.0);

	// Viewer to fragment
	vec3 V = fragPos - ubo.camPos.xyz;

	// Diffuse part
	L = normalize(L);
	vec3 N = normalize(normal);
	float LdotN = max(0.0, dot(L, N));
	vec3 diff = ubo.lights[light].color.rgb * albedo * LdotN;

	// Specular part
	vec3 R = reflect(-L, N);
	V = normalize(V);
	float RdotV = max(0.0, dot(R, V));
	vec3 spec = ubo.lights[light].color.rgb * specular * pow(RdotV, 32.0);
	
	// Attenuation
	float atten = 1.0 / (1.0 + ubo.lights[light].attenuation.x * pow(dist, 2));

	return atten * (diff + spec);
}