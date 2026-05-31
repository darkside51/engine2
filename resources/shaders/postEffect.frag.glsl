#version 450

layout (constant_id = 0) const uint shadowCascades = 2;

/*
layout(set = 0, binding = 0) uniform sampler2D texture0;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(texture0, v_uv);
	out_color.rgb = pow(out_color.rgb, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
}
*/

#import common
#import lighting
#import blur
#import colors
#import pbr

layout(set = 0, binding = 0) uniform sampler2D textureSsao;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D textureEmissive;
layout(set = 0, binding = 4) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 5) uniform sampler2D textureMetallicRoughness;
//layout(set = 0, binding = 6) uniform sampler2DArrayShadow textureShadowMap;
layout(set = 0, binding = 6) uniform sampler2DArray textureShadowMap;

layout (set = 0, binding = 7) uniform shadowsUniformBuffer {
	mat4 cascades[shadowCascades];
	vec4 splits;
	vec4 options;
} u_shadow;

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

const float biases[4] = float[](0.0001f, 0.0004f, 0.0001f, 0.0002f);

void main() {
	const float occlusion = push.options.x > 0.0f ? radialBlur(textureSsao, v_uv, int(push.options.x - 1.0f)) : 1.0f;
	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0f - 1.0f;

	const vec2 metallicRoughness = texture(textureMetallicRoughness, v_uv).rg;

	/*const*/ vec4 albedo = texture(textureAlbedo, v_uv);
	const vec4 emissive = texture(textureEmissive, v_uv);

	const float linearDepth = texture(texturePositionDepth, v_uv).r;
    const vec3 cameraPosition = push.view[3].xyz;

	const vec4 worldPosition = vec4(reconstructPosition(v_rayDirWorld, cameraPosition, linearDepth), 1.0f);
	const float viewDepth = reconstructPosition(v_rayDirView, vec3(0.0f), linearDepth).z;

	// shadows
	// get cascade index for the current fragment's view position
	uint cascade = 0;
	for (uint i = 0; i < shadowCascades - 1; ++i) {
		if (viewDepth < u_shadow.splits[i]) {	
			cascade = i + 1;
		}
	}

	vec4 shadowCoord = u_shadow.cascades[cascade] * worldPosition;
	shadowCoord = shadowCoord / shadowCoord.w;
	shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f;
	
	const float NdotL = max(0.0f, dot(normal, -push.light.xyz));

	const float bias = max(biases[2 * cascade + 1] * (1.0f - abs(NdotL)), biases[2 * cascade]);
	//const float shadowValue = textureProj(textureShadowMap, shadowCoord, vec2(0.0f), cascade, bias, u_shadow.options.x); // hardware pcf if use sampler2DArrayShadow
	const float shadowValue = filterPCF(textureShadowMap, shadowCoord, cascade, bias, u_shadow.options.x);

	const vec3 ambient = push.ambient.xyz * push.ambient.w * occlusion;

// pbr
	const vec3 N = normal;
	const vec3 V = vec3(0.0f, 0.0f, 1.0f);
	const vec3 L = -push.light.xyz;

	//vec3 V = normalize(cameraPosition - worldPosition.xyz);
	//vec3 L = normalize(100.0f * -push.light.xyz - worldPosition.xyz);

	const vec3 Lo = BRDF(L, V, N, metallicRoughness.x, metallicRoughness.y, push.directionalLight.xyz * push.directionalLight.w, albedo.rgb);

	// combine with ambient
	vec3 baseColor = albedo.xyz * ambient;
	baseColor += max(Lo, vec3(0.0));
	baseColor *= shadowValue;
	baseColor += emissive.rgb;
	baseColor = mix(baseColor, vec3(1.0f), step(0.0f, viewDepth));
//

	//vec3 viewDir = vec3(0.0f, 0.0f, 1.0f);
	//vec3 reflect = reflect(push.light.xyz, normal);
	//vec3 specular = pow(max(dot(reflect, viewDir), 0.0f), 16.0f) * metallicRoughness.r * push.directionalLight.xyz;

	//const vec3 light = shadowValue * (ambient + NdotL * (push.directionalLight.xyz * push.directionalLight.w));
	//vec3 baseColor = mix(light * albedo.rgb + specular + emissive.rgb, vec3(1.0f), step(0.0f, viewDepth));
	
	// saturation
	baseColor = saturate(baseColor, push.options.z);
	// gamma
	vec3 color = pow(baseColor, vec3(0.4545f)); // gamma correction 1.0f / 2.2f = 0.4545f

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

		if (push.options.y > 5.0f && push.options.y < 9.0f) {
			const float shadowDepth = texture(textureShadowMap, vec3(v_uv, clamp(int(push.options.y - 6.0f), 0.0f, 2.0f))).r;
			//const float shadowDepth = texture(textureShadowMap, vec4(v_uv, clamp(int(push.options.y - 6.0f), 0.0f, 2.0f), 0.0f));
			out_color = mix(out_color, vec4(shadowDepth, shadowDepth, shadowDepth, 1.0f), step(5.1f, push.options.y) * step(push.options.y, 7.9f));
		}

		if (push.options.y > 7.0f) {
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

	/*
	//float r = texture(textureAlbedo, v_uv - 0.001f).r;
    //float g = texture(textureAlbedo, v_uv - 0.001f).g;
    //float b = texture(textureAlbedo, v_uv + 0.001f).b;
    //const vec4 albedo = vec4(r, g, b, 1.0f);

	// 2. Contrast
    //color = pow(color, vec3(1.1,  1.1, 1.1));
    
    // 3. Saturation
    //float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    //color = mix(luminance.xxx, color, 0.75);
    
    // 4. Filmic Tonemapping (Reinhard)
    //color = color / (color + 1.0);
	*/
}