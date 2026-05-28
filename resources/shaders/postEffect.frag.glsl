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

layout(set = 0, binding = 0) uniform sampler2D textureSsao;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D textureEmissive;
layout(set = 0, binding = 4) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 5) uniform sampler2DArrayShadow textureShadowMap;
//layout(set = 0, binding = 5) uniform sampler2DArray textureShadowMap;

layout (set = 0, binding = 6) uniform shadowsUniformBuffer {
	mat4 cascades[shadowCascades];
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


void main() {
	const bool aoEnable = push.options.x > 0.0f;

	const float occlusion = aoEnable ? radialBlur(textureSsao, v_uv, int(push.options.x - 1.0f)) : 1.0f;
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
	for (uint i = 0; i < shadowCascades - 1; ++i) {
		if (viewDepth < u_shadow.splits[i]) {	
			cascade = i + 1;
		}
	}

	vec4 shadowCoord = u_shadow.cascades[cascade] * worldPosition;
	shadowCoord = shadowCoord / shadowCoord.w;
	shadowCoord.xy = (shadowCoord.xy + 1.0) * 0.5;
	
	const float shadowValue = textureProj(textureShadowMap, shadowCoord, vec2(0.0f), cascade, push.ambient.w); // hardware pcf if use sampler2DArrayShadow
	//const float shadowValue = filterPCF(textureShadowMap, shadowCoord, cascade, push.ambient.w);

	vec3 viewDir = vec3(1.0, 0.0, 0.0);
	vec3 reflect = reflect(push.light.xyz, normal);
	vec3 specular = vec3(0.0);//pow(max(dot(reflect, viewDir), 0.0), 8.0) * vec3(1.0);

	const float NdotL = max(0.0f, dot(normal, -push.light.xyz));
	const vec3 ambient = push.ambient.xyz * push.ambient.w * occlusion;
	const vec3 baseColor = shadowValue * (ambient + NdotL * (push.directionalLight.xyz * push.directionalLight.w)) * albedo.rgb + specular + emissive.rgb;
	vec3 color = pow(baseColor, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
	
	// vignette
	if (push.options.w > 0.0f) {
		const vec2 uv =  v_uv * (1.0f - v_uv.yx);
	    const float vig = pow(uv.x*uv.y * 15.0f, push.options.w); // multiply with sth for intensity; change push.options.w for modifying the extend of the  vignette
	    color *= vig;
	}
	out_color = vec4(color, 1.0f);

	if (push.options.y > 0.0f) {
		out_color = mix(out_color, vec4(occlusion, occlusion, occlusion, 1.0), step(0.1, push.options.y) * step(push.options.y, 1.1));
		out_color = mix(out_color, emissive, step(1.1, push.options.y) * step(push.options.y, 2.1));
		out_color = mix(out_color, worldPosition, step(2.1, push.options.y) * step(push.options.y, 3.1));

		
		if (push.options.y > 3.0f && push.options.y < 7.0) {
			//const float shadowDepth = texture(textureShadowMap, vec3(v_uv, clamp(int(push.options.y - 4.0), 0, 2))).r;
			const float shadowDepth = texture(textureShadowMap, vec4(v_uv, clamp(int(push.options.y - 4.0), 0, 2), 0.0));
			out_color = mix(out_color, vec4(shadowDepth, shadowDepth, shadowDepth, 1.0f), step(3.1, push.options.y) * step(push.options.y, 5.9));
		}
		

		if (push.options.y > 5.0) {
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