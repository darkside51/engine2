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

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Params {
	vec4 lightDir;
} push;

float ssaoBlur(int blurRange) {
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

void main() {
	const float occlusion = ssaoBlur(1);
	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;
	const vec4 albedo = texture(textureAlbedo, v_uv);

	const float NdotL = max(0.1, dot(normal, push.lightDir.xyz));
	vec3 baseColor = albedo.rgb * NdotL;

	out_color = vec4(pow(baseColor, vec3(0.4545)) * occlusion, 1.0); // gamma correction 1.0 / 2.2 = 0.4545
	//out_color = vec4(occlusion, occlusion, occlusion, 1.0);
	//out_color = vec4(normal, 1.0);

	//out_color = vec4(baseColor * occlusion, 1.0);
	//out_color.rgb = pow(out_color.rgb, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
}