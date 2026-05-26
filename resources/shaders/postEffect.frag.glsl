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
layout(set = 0, binding = 5) uniform sampler2DArrayShadow textureShadowMap;

layout (set = 0, binding = 6) uniform shadowsUniformBuffer {
	mat4 cascades[3];
	vec4 splits;
} u_shadow;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_rayDir;
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
	float shadow = 1.0;
	float bias = 0.005;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
		float dist = texture(textureShadowMap, vec4(shadowCoord.st + offset, cascadeIndex, shadowCoord.z)).r;
		if (shadowCoord.w > 0.0 && dist < 0.001) {
			shadow = 0.2;
		}
	}
	return shadow;

}

void main() {
	const bool aoEnable = push.options.x > 0.0f;

	const float occlusion = aoEnable ? ssaoRadialBlur(int(push.options.x - 1.0f)) : 1.0f;
	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;
	const vec4 albedo = texture(textureAlbedo, v_uv);
	const vec4 emissive = texture(textureEmissive, v_uv);

	const float linearDepth = texture(texturePositionDepth, v_uv).r;
    const vec3 cameraPosition = push.view[3].xyz;

	const vec4 worldPosition = vec4(reconstructPosition(v_rayDir, cameraPosition, linearDepth), 1.0f);
	//const vec4 worldPosition = texture(texturePositionDepth, v_uv);

	// shadows
	// get cascade index for the current fragment's view position
	uint cascade = 0;
	//for (uint i = 0; i < (3 - 1); ++i) {
	//	if (in_view_depth < u_shadow.splits[i]) {	
	//		cascade = i + 1;
	//	}
	//}

	vec4 shadowCoord = u_shadow.cascades[cascade] * worldPosition;
	shadowCoord.xy = (shadowCoord.xy + 1.0) * 0.5;
	shadowCoord.xyz = shadowCoord.xyz / shadowCoord.w;

	const float shadowValue = textureProj(shadowCoord, vec2(0.0f), cascade);//1.0f - texture(textureShadowMap, vec4(shadowCoord.xy, cascade, shadowCoord.z)).r;
	//const float shadowValue = 1.0f - texture(textureShadowMap, vec4(shadowCoord.xy, cascade, shadowCoord.z)).r;
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

	const float shadowDepth = texture(textureShadowMap, vec4(v_uv, clamp(int(push.options.y - 4.0), 0, 2), 0.0)).r;
	out_color = mix(out_color, vec4(shadowDepth, shadowDepth, shadowDepth, 1.0f), step(3.1, push.options.y) * step(push.options.y, 7.1));

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
}