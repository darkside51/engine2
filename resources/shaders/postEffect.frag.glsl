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

layout(set = 0, binding = 0) uniform sampler2D textureSsao;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D textureEmissive;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Params {
	vec4 lightDir;
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

vec3 emissiveRadialBlur(int blurRange, int step) {
	int n = 0;
	const vec2 texelSize = 1.0 / vec2(textureSize(textureEmissive, 0));
	vec3 result = vec3(0.0);
	for (int x = -blurRange; x <= blurRange; x += step) {
		for (int y = -blurRange; y <= blurRange; y += step) {
			const vec2 offset = vec2(float(x), float(y)) * texelSize;
			result += texture(textureEmissive, v_uv + offset).rgb;
			n++;
		}
	}
	return result / float(n);
}

void main() {
	const float occlusion = ssaoRadialBlur(1);
	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;
	const vec4 albedo = texture(textureAlbedo, v_uv);
	const vec4 emissive = texture(textureEmissive, v_uv);//4.0 * emissiveRadialBlur(16, 2);

	vec3 viewDir = vec3(1.0, 0.0, 0.0);
	vec3 reflect = reflect(push.lightDir.xyz, normal);
	vec3 specular = vec3(0.0);//pow(max(dot(reflect, viewDir), 0.0), 8.0) * vec3(1.0);

	const float ambient = 0.05;
	const float NdotL = max(0.0, dot(normal, push.lightDir.xyz));
	const vec3 baseColor = albedo.rgb * (NdotL + ambient) + specular + emissive.rgb;
	const vec3 clearColor = vec3(0.5f);
	out_color = vec4(mix(pow(baseColor, vec3(0.4545)) * occlusion, clearColor, step(albedo.w, 0.0)), 1.0); // gamma correction 1.0 / 2.2 = 0.4545

	//out_color = vec4(mix(pow(baseColor * occlusion, vec3(0.4545)), clearColor, step(albedo.w, 0.0)), 1.0); // gamma correction 1.0 / 2.2 = 0.4545
	//out_color = vec4(occlusion, occlusion, occlusion, 1.0);
	//out_color = vec4(normal, 1.0);
	//out_color = emissive;
}