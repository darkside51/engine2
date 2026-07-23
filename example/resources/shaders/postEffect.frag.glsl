#version 450

#include "common"
#include "lighting"
#include "blur"
#include "colors"
#include "pbr"

layout (constant_id = 0) const uint shadowCascades = 2;

layout(set = 0, binding = 0) uniform sampler2D textureSsao;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D textureEmissive;
layout(set = 0, binding = 4) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 5) uniform sampler2D textureMetallicRoughness;
layout(set = 0, binding = 6) uniform sampler2DArrayShadow textureShadowMap;
//layout(set = 0, binding = 6) uniform sampler2DArray textureShadowMap;

layout (set = 0, binding = 7) uniform shadowsUniformBuffer {
	mat4 cascades[shadowCascades];
	mat4 viewInverse;
	vec4 splits;
	vec4 texelSizes;
	vec4 options;
} u_shadow;

//layout(set = 1, binding = 0) uniform samplerCube textureCubeMap;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_rayDirWorld;
layout (location = 2) in vec3 v_rayDirView;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Params {
	mat4 view;
	vec4 ambient;
	vec4 directionalLight;
	vec4 light;
	vec4 options;
} push;

//const float biases[4] = float[](0.0015f, 0.0025f, 0.0009f, 0.0015f); // count = shadowCascades * 2
const float biases[4] = float[](0.0001f, 0.0004f, 0.0001f, 0.0002f); // count = shadowCascades * 2

void main() {
	const float occlusion = push.options.x > 0.0f ? radialBlur(textureSsao, v_uv, int(push.options.x - 1.0f)) : 1.0f;
	const vec3 normal = normalize(texture(textureNormal, v_uv).rgb * 2.0f - 1.0f);

	const vec2 metallicRoughness = texture(textureMetallicRoughness, v_uv).rg;

	/*const*/ vec4 albedo = texture(textureAlbedo, v_uv);
	const vec4 emissive = texture(textureEmissive, v_uv);

	const float linearDepth = texture(texturePositionDepth, v_uv).r;
    const vec3 cameraPosition = push.view[3].xyz;

	const vec4 worldPosition = vec4(reconstructPosition(v_rayDirWorld, cameraPosition, linearDepth), 1.0f);
	const vec3 viewPosition = reconstructPosition(v_rayDirView, vec3(0.0f), linearDepth);

	// pbr
	const vec3 N = normal; // in view space
    const vec3 V = normalize(-v_rayDirView); // in view space
    const vec3 L = normalize(-push.light.xyz); // in view space

    // per-light calculation (Loop for multiple lights)
    const vec3 H = normalize(V + L);

    const float dotNL = max(dot(N, L), 0.0f);
    const float dotNV = max(dot(N, V), 0.0f);
    const float dotNH = max(dot(N, H), 0.0f);
    const float dotHV = max(dot(H, V), 0.0f);

    // shadows
	// get cascade index for the current fragment's view position
	uint cascade = 0;
	for (uint i = 0; i < shadowCascades - 1; ++i) {
		if (viewPosition.z < u_shadow.splits[i]) {	
			cascade = i + 1;
		}
	}

	float texelSize = u_shadow.texelSizes[cascade];
	//vec4 shadowCoord = u_shadow.cascades[cascade] * worldPosition;

	//vec3 worldNormal = normalize((u_shadow.viewInverse * vec4(normal, 0.0f)).xyz);

	float factor = 0.001;
	vec3 biasedViewPos = viewPosition + (normal * factor);
	vec4 shadowCoord = u_shadow.cascades[cascade] * vec4(biasedViewPos, 1.0f);

	//vec3 biasedWorldPos = worldPosition.xyz + (worldNormal * factor);
	//vec4 shadowCoord = u_shadow.cascades[cascade] * vec4(biasedWorldPos, 1.0f);
	
	shadowCoord.xyz /= shadowCoord.w;
	shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f;
	
	const ivec2 texSize = textureSize(textureShadowMap, 0).xy;
	//const float bias = max(0.002f * (1.0f - dotNL), 0.0002f);
	const float bias = 0.0f;

	//shadowCoord.y = shadowCoord.y + bias;

	//const float shadowValue = textureProj(textureShadowMap, shadowCoord, vec2(0.0f), cascade, bias, u_shadow.options.x); // hardware pcf if use sampler2DArrayShadow
	//const float shadowValue = filterPCF(textureShadowMap, shadowCoord, cascade, bias, u_shadow.options.x);
	//const float shadowValue = clamp(filterPCF_Low2(textureShadowMap, texSize, shadowCoord.xyz, cascade, bias, 1.5f, 4), u_shadow.options.x, 1.0f);
	const float shadow = filterPCF_Low(textureShadowMap, texSize, shadowCoord.xyz, cascade, bias);
	const float shadowValue = clamp(shadow, u_shadow.options.x, 1.0f);
	//const float shadowValue = textureProj2(textureShadowMap, shadowCoord, vec2(0.0f), cascade, bias, u_shadow.options.x);
	// shadows

	const float metallic = metallicRoughness.x;
	const float roughness = metallicRoughness.y;

    // calculate base reflectance at normal incidence (dielectric vs metal)
    vec3 F0 = vec3(0.04f); 
    F0 = mix(F0, albedo.rgb, metallic);

    // reflectance equation setup
    vec3 Lo = vec3(0.0f);
    
    const float distance = 1.0f;//length(lightPosition - worldPosition.xyz);
    const float attenuation = 1.0f / (distance * distance); // Physical attenuation
    const vec3 radiance = push.directionalLight.xyz * push.directionalLight.w * attenuation;

     // Cook-Torrance BRDF Evaluation
    const float D = distributionGGX(dotNH, roughness);
    const float G = geometrySmith(dotNV, dotNL, roughness);      
    const vec3 F = fresnelSchlick(dotHV, F0);

    const vec3 numerator = D * G * F;
    const float denominator = 4.0f * dotNV * dotNL + 0.0001f; // avoid division by zero
    //const 
    vec3 specular = numerator / denominator;
	specular *= step(0.5f, shadowValue); // no specular in shadow
    
    // energy conservation: diffuse must equal remaining non-reflected energy
    const vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - metallic;	 // Metals absorb refracted light completely

    Lo += (kD * albedo.rgb / PI + specular) * radiance * dotNL;

    // ambient lighting (Placeholder: upgrade to Image-Based Lighting later)
    //const vec3 ambient = mix(vec3(0.25, 1.0, 0.0), vec3(1.0, 1.0, 1.0), clamp(worldPosition.z * 6.2f, 0.0f, 1.0f)) * push.ambient.xyz * push.ambient.w * albedo.rgb * occlusion;
    const vec3 ambient = push.ambient.xyz * push.ambient.w * albedo.rgb * occlusion;
    vec3 baseColor = ambient + Lo;

    //baseColor *= shadowValue;
    baseColor = clamp(baseColor * shadowValue, ambient, baseColor);
    baseColor += emissive.rgb;
	//

	// 1. contrast
    baseColor = pow(baseColor, vec3(1.1f));

	// 2. saturation
	baseColor = saturate(baseColor, push.options.z);

	// 3. compress HDR to [0.0f, 1.0f]
	baseColor = baseColor / (baseColor + vec3(1.0f)); // Reinhard Tonemapping to map

	// skybox
	//const vec3 cubeColor = texture(textureCubeMap, v_rayDirWorld).rgb;
	//baseColor = mix(baseColor, cubeColor, step(0.0f, viewPosition.z));

	// 4. gamma
	vec3 color = pow(baseColor, vec3(0.4545f)); // gamma correction 1.0f / 2.2f = 0.4545f
	color = mix(color, albedo.rgb, step(0.0f, viewPosition.z));

	// vignette
	if (push.options.w > 0.0f) {
		const vec2 uv =  v_uv * (1.0f - v_uv.yx);
	    const float vig = pow(uv.x*uv.y * 15.0f, push.options.w); // multiply with sth for intensity; change push.options.w for modifying the extend of the  vignette
	    color *= vig;
	}
	out_color = vec4(color, 1.0f);

	if (push.options.y > 0.0f) {
		albedo.rgb = pow(albedo.rgb, vec3(0.4545f));
		out_color = mix(out_color, vec4(occlusion, occlusion, occlusion, 1.0f), step(0.1f, push.options.y) * step(push.options.y, 1.1f));
		out_color = mix(out_color, albedo, step(1.1f, push.options.y) * step(push.options.y, 2.1f));
		out_color = mix(out_color, emissive, step(2.1f, push.options.y) * step(push.options.y, 3.1f));
		out_color = mix(out_color, vec4(metallicRoughness, 0.0f, 1.0f), step(3.1f, push.options.y) * step(push.options.y, 4.1f));
		out_color = mix(out_color, worldPosition, step(4.1f, push.options.y) * step(push.options.y, 5.1f));
		out_color = mix(out_color, vec4(normal * 0.5f + 0.5f, 1.0f), step(5.1f, push.options.y) * step(push.options.y, 6.1f));

		if (push.options.y > 6.0f && push.options.y < 10.0f) {
			//const float shadowDepth = texture(textureShadowMap, vec3(v_uv, clamp(int(push.options.y - 7.0f), 0.0f, 2.0f))).r;
			const float shadowDepth = texture(textureShadowMap, vec4(v_uv, clamp(int(push.options.y - 7.0f), 0.0f, 2.0f), 0.0f));
			out_color = mix(out_color, vec4(shadowDepth, shadowDepth, shadowDepth, 1.0f), step(6.1f, push.options.y) * step(push.options.y, 8.9f));
		}

		if (push.options.y > 8.0f) {
			switch(cascade) {
				case 0 : 
					out_color.rgb *= vec3(1.0f, 0.25f, 0.25f);
					break;
				case 1 : 
					out_color.rgb *= vec3(0.25f, 1.0f, 0.25f);
					break;
				case 2 : 
					out_color.rgb *= vec3(0.25f, 0.25f, 1.0f);
					break;
			}
		}
	}
}