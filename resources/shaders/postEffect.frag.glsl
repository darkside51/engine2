#version 450

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

layout(set = 0, binding = 0) uniform sampler2D textureSsao;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D textureEmissive;
layout(set = 0, binding = 4) uniform sampler2D texturePositionDepth;
//layout(set = 0, binding = 5) uniform sampler2DArrayShadow textureShadowMap;
layout(set = 0, binding = 5) uniform sampler2DArray textureShadowMap;

layout (set = 0, binding = 6) uniform shadowsUniformBuffer {
	mat4 cascades[2];
	vec4 splits;
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

float ssaoRadialBlur(int blurRange) {
	int n = 0;
	const vec2 texelSize = 1.0 / vec2(textureSize(textureSsao, 0));
	float result = 0.0;
	for (int x = -blurRange; x <= blurRange; x++) {
		for (int y = -blurRange; y <= blurRange; y++) {
			const vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(textureSsao, v_uv + offset).r;
			n++;
		}
	}
	return result / float(n);
}

float textureProj(vec4 shadowCoord, vec2 offset, uint cascadeIndex) {
	float shadow = 1.0f;
	const float bias = 0.001f;

	if (shadowCoord.z > -1.0f && shadowCoord.z < 1.0f) {
		//const float dist = texture(textureShadowMap, vec4(shadowCoord.xy + offset, cascadeIndex, shadowCoord.z + bias));
		//if (shadowCoord.w > 0.0 && dist < bias) {
		//	shadow = clamp(push.ambient.w + dist, 0.0f, 1.0f);
		//}
		const float dist = texture(textureShadowMap, vec3(shadowCoord.xy + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist > shadowCoord.z + bias) {
			shadow = push.ambient.w;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, uint cascadeIndex) {
	const ivec2 texDim = textureSize(textureShadowMap, 0).xy;
	const float scale = 0.65;
	const float dx = scale * 1.0 / float(texDim.x);
	const float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	const int range = 1;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

void main() {
	const bool aoEnable = push.options.x > 0.0f;

	const float occlusion = aoEnable ? ssaoRadialBlur(int(push.options.x - 1.0f)) : 1.0f;
	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;
	const vec4 albedo = texture(textureAlbedo, v_uv);
	const vec4 emissive = texture(textureEmissive, v_uv);

	const float linearDepth = texture(texturePositionDepth, v_uv).r;
    const vec3 cameraPosition = push.view[3].xyz;

	const vec4 worldPosition = vec4(reconstructPosition(v_rayDirWorld, cameraPosition, linearDepth), 1.0f);
	const float viewDepth = reconstructPosition(v_rayDirView, vec3(0.0f), linearDepth).z;
	//const vec4 worldPosition = texture(texturePositionDepth, v_uv);

	// shadows
	// get cascade index for the current fragment's view position
	uint cascade = 0;
	for (uint i = 0; i < 1; ++i) {
		if (viewDepth < u_shadow.splits[i]) {	
			cascade = i + 1;
		}
	}

	vec4 shadowCoord = u_shadow.cascades[cascade] * worldPosition;
	shadowCoord = shadowCoord / shadowCoord.w;
	shadowCoord.xy = (shadowCoord.xy + 1.0) * 0.5;
	
	//const float shadowValue = 1.0f - texture(textureShadowMap, vec4(shadowCoord.xy, cascade, shadowCoord.z)).r;
	//const float shadowValue = textureProj(shadowCoord, vec2(0.0f), cascade);
	const float shadowValue = filterPCF(shadowCoord, cascade);
	//

	vec3 viewDir = vec3(1.0, 0.0, 0.0);
	vec3 reflect = reflect(push.light.xyz, normal);
	vec3 specular = vec3(0.0);//pow(max(dot(reflect, viewDir), 0.0), 8.0) * vec3(1.0);

	const float NdotL = max(0.0f, dot(normal, -push.light.xyz));
	const vec3 baseColor = shadowValue * (push.ambient.xyz * push.ambient.w + NdotL * (push.directionalLight.xyz * push.directionalLight.w)) * albedo.rgb + specular + emissive.rgb;

	//const vec3 baseColor = albedo.rgb * (NdotL + ambient) + specular + emissive.rgb;
	const vec3 color = pow(baseColor * occlusion, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
	out_color = vec4(color, 1.0f);
	//const vec3 clearColor = pow(vec3(0.5f) + emissive.rgb, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
	//out_color = vec4(mix(color, clearColor, step(albedo.w, 0.0)), 1.0); 

	//out_color = vec4(mix(pow(baseColor, vec3(0.4545)) * occlusion, clearColor, step(albedo.w, 0.0)), 1.0); // gamma correction 1.0 / 2.2 = 0.4545

	out_color = mix(out_color, vec4(occlusion, occlusion, occlusion, 1.0), step(0.1, push.options.y) * step(push.options.y, 1.1));
	out_color = mix(out_color, emissive, step(1.1, push.options.y) * step(push.options.y, 2.1));
	out_color = mix(out_color, worldPosition, step(2.1, push.options.y) * step(push.options.y, 3.1));

	
	if (push.options.y > 3.0f && push.options.y < 7.0) {
		const float shadowDepth = texture(textureShadowMap, vec3(v_uv, clamp(int(push.options.y - 4.0), 0, 2))).r;
		//texture(textureShadowMap, vec4(v_uv, clamp(int(push.options.y - 4.0), 0, 2), 0.0)).r;
		out_color = mix(out_color, vec4(shadowDepth, shadowDepth, shadowDepth, 1.0f), step(3.1, push.options.y) * step(push.options.y, 6.9));
	}
	
	// vignette
	if (push.options.w > 0.0f) {
		const vec2 uv =  v_uv * (1.0f - v_uv.yx);
    	const float vig = pow(uv.x*uv.y * 15.0f, push.options.w); // multiply with sth for intensity; change push.options.w for modifying the extend of the  vignette
    	out_color.rgb *= vig;
    }

	//out_color = vec4(mix(pow(baseColor * occlusion, vec3(0.4545)), clearColor, step(albedo.w, 0.0)), 1.0); // gamma correction 1.0 / 2.2 = 0.4545
	//out_color = vec4(occlusion, occlusion, occlusion, 1.0);
	//out_color = vec4(normal, 1.0);
	//out_color = emissive;
	//out_color = texture(textureWorldPosition, v_uv);

	if (push.options.y > 6.0) {
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