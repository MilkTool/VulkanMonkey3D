#ifndef LIGHT_H_
#define LIGHT_H_

#include "../Common/tonemapping.glsl"
#include "../Common/common.glsl"
#include "pbr.glsl"
#include "Material.glsl"

struct Light {
	vec4 color; // .a is the intensity
	vec4 position;
};


layout(push_constant) uniform SS { vec4 effect; vec4 effect1; mat4 invViewProj; vec4 effect2; } screenSpace;
layout(constant_id = 0) const int NUM_LIGHTS = 1;
layout(set = 0, binding = 0) uniform sampler2D samplerDepth;
layout(set = 0, binding = 1) uniform sampler2D samplerNormal;
layout(set = 0, binding = 2) uniform sampler2D samplerAlbedo;
layout(set = 0, binding = 3) uniform sampler2D samplerMetRough;
layout(set = 0, binding = 4) uniform UBO { vec4 camPos; Light lights[NUM_LIGHTS + 1]; } ubo;
layout(set = 0, binding = 5) uniform sampler2D ssaoBlurSampler;
layout(set = 0, binding = 6) uniform sampler2D ssrSampler;
layout(set = 0, binding = 7) uniform sampler2D emiSampler;
layout(set = 0, binding = 8) uniform sampler2D lutIBLSamlpler;
layout(set = 1, binding = 1) uniform sampler2DShadow shadowMapSampler0;
layout(set = 2, binding = 1) uniform sampler2DShadow shadowMapSampler1;
layout(set = 3, binding = 1) uniform sampler2DShadow shadowMapSampler2;
layout(set = 4, binding = 1) uniform samplerCube cubemapSampler;

vec3 compute_point_light(int lightIndex, Material material, vec3 world_pos, vec3 camera_pos, vec3 material_normal, float ssao)
{
	vec3 light_dir_full = world_pos - ubo.lights[lightIndex].position.xyz;
	float light_dist = max(0.1, length(light_dir_full));
	if (light_dist > screenSpace.effect2.z) // max range
		return vec3(0.0);

	vec3 light_dir = normalize(-light_dir_full);
	float attenuation = light_dist * light_dist;
	vec3 point_color = ubo.lights[lightIndex].color.xyz / attenuation;
	point_color *= screenSpace.effect2.y * ssao; // intensity

	float roughness = material.roughness * 0.75 + 0.25;

	vec3 L = light_dir;
	vec3 V = normalize(camera_pos - world_pos);
	vec3 H = normalize(V + L);
	vec3 N = material_normal;

	float NoV = clamp(dot(N, V), 0.001, 1.0);
	float NoL = clamp(dot(N, L), 0.001, 1.0);
	float HoV = clamp(dot(H, V), 0.001, 1.0);
	float LoV = clamp(dot(L, V), 0.001, 1.0);

	vec3 F0 = compute_F0(material.albedo, material.metallic);
	vec3 specular_fresnel = fresnel(F0, HoV);
	vec3 specref = NoL * cook_torrance_specular(N, H, NoL, NoV, specular_fresnel, roughness);
	vec3 diffref = NoL * (1.0 - specular_fresnel) * (1.0 / PI);

	vec3 reflected_light = specref;
	vec3 diffuse_light = diffref * material.albedo * (1.0 - material.metallic);

	vec3 res = point_color * (reflected_light + diffuse_light);

	return res;
}

/*
Copyright(c) 2016-2019 Panos Karabelas
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

float3 SampleEnvironment(samplerCube tex_environment, float3 dir, float mipLevel)
{
	ivec2 texDim = textureSize(tex_environment, int(mipLevel));
	float2 resolution = vec2(float(texDim.x), float(texDim.y));
	float2 texelSize = 1.0 / resolution;

	float dx 			= 1.0f * texelSize.x;
	float dy 			= 1.0f * texelSize.y;
	float dz 			= 0.5f * (texelSize.x + texelSize.y);

	float3 d0 			= textureLod(tex_environment, dir + float3(0.0, 0.0, 0.0), mipLevel).rgb;
	float3 d1 			= textureLod(tex_environment, dir + float3(-dx, -dy, -dz), mipLevel).rgb;
	float3 d2			= textureLod(tex_environment, dir + float3(-dx, -dy, +dz), mipLevel).rgb;
	float3 d3			= textureLod(tex_environment, dir + float3(-dx, +dy, +dz), mipLevel).rgb;
	float3 d4			= textureLod(tex_environment, dir + float3(+dx, -dy, -dz), mipLevel).rgb;
	float3 d5			= textureLod(tex_environment, dir + float3(+dx, -dy, +dz), mipLevel).rgb;
	float3 d6			= textureLod(tex_environment, dir + float3(+dx, +dy, -dz), mipLevel).rgb;
	float3 d7			= textureLod(tex_environment, dir + float3(+dx, +dy, +dz), mipLevel).rgb;

	return (d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7) * 0.125f;
}

float3 GetSpecularDominantDir(float3 normal, float3 reflection, float roughness)
{
	const float smoothness = 1.0f - roughness;
	const float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
	return lerp(normal, reflection, lerpFactor);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	float smoothness = 1.0 - roughness;
    return F0 + (max(float3(smoothness, smoothness, smoothness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

// https://www.unrealengine.com/blog/physically-based-shading-on-mobile
float3 EnvBRDFApprox(float3 specColor, float roughness, float NdV)
{
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f );
    const float4 c1 = float4(1.0f, 0.0425f, 1.0f, -0.04f );
    float4 r 		= roughness * c0 + c1;
    float a004 		= min(r.x * r.x, exp2(-9.28f * NdV)) * r.x + r.y;
    float2 AB 		= float2(-1.04f, 1.04f) * a004 + r.zw;
    return specColor * AB.x + AB.y;
}

float3 ImageBasedLighting(Material material, float3 normal, float3 camera_to_pixel, samplerCube tex_environment, sampler2D tex_lutIBL)
{
	float3 reflection 	= reflect(camera_to_pixel, normal);
	// From Sebastien Lagarde Moving Frostbite to PBR page 69
	reflection	= GetSpecularDominantDir(normal, reflection, material.roughness);

	float NdV 	= saturate(dot(-camera_to_pixel, normal));
	float3 F 	= FresnelSchlickRoughness(NdV, material.F0, material.roughness);

	float3 kS 	= F; 			// The energy of light that gets reflected
	float3 kD 	= 1.0f - kS;	// Remaining energy, light that gets refracted
	kD 			*= 1.0f - material.metallic;	

	// Diffuse
	float3 irradiance	= SampleEnvironment(tex_environment, normal, 8);
	float3 cDiffuse		= irradiance * material.albedo;

	// Specular
	float mipLevel 			= max(0.001f, material.roughness * material.roughness) * 11.0f; // max lod 11.0f
	float3 prefilteredColor	= SampleEnvironment(tex_environment, reflection, mipLevel);
	float2 envBRDF  		= texture(tex_lutIBL, float2(NdV, material.roughness)).xy;
	float3 cSpecular 		= prefilteredColor * (F * envBRDF.x + envBRDF.y);

	return kD * cDiffuse + cSpecular; 
}

#endif