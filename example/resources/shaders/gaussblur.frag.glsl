#version 450

layout (set = 0, binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform Params {
	// x - blur radius scale, y - bloom intensity (applied on the last pass only),
	// z - direction (1.0 - horizontal, 0.0 - vertical), w - bright pass threshold (first pass only)
	vec4 scaleStrengthDirection;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

// 13-tap separable gaussian (radius = 6 texels, sigma = 3), weights sum to 1.0
const int kRadius = 6;
const float weights[kRadius + 1] = float[](
	0.137023, 0.129618, 0.109719, 0.083108, 0.056332, 0.034167, 0.018544
);

vec3 brightPass(vec3 color) {
	const float threshold = push.scaleStrengthDirection.w;
	const float brightness = max(max(color.r, color.g), color.b);
	// soft knee; with threshold == 0.0 it behaves as an identity
	const float knee = max(threshold * 0.5f, 0.001f);
	return color * smoothstep(threshold, threshold + knee, brightness);
}

void main() {
	const vec2 texel = 1.0f / vec2(textureSize(u_texture, 0)) * push.scaleStrengthDirection.x;
	const vec2 dir = push.scaleStrengthDirection.z > 0.5f ? vec2(texel.x, 0.0f) : vec2(0.0f, texel.y);

	vec3 result = brightPass(texture(u_texture, v_uv).rgb) * weights[0];
	for (int i = 1; i <= kRadius; ++i) {
		const float offset = float(i);
		result += brightPass(texture(u_texture, v_uv + dir * offset).rgb) * weights[i];
		result += brightPass(texture(u_texture, v_uv - dir * offset).rgb) * weights[i];
	}

	// HDR intensity keeps the glow visible after Reinhard tonemapping
	out_color = vec4(max(result, vec3(0.0f)) * push.scaleStrengthDirection.y, 1.0f);
}
