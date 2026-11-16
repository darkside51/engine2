#version 450

layout (set = 0, binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform Params {
	// x - blur radius scale, y - bloom intensity (applied on the last pass only),
	// z - direction (1.0 - horizontal, 0.0 - vertical), w - bright pass threshold (first pass only)
	vec4 scaleStrengthDirection;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

// Предвычисленные веса и смещения для 9-tap ядра через 5 выборок
const float weights[3] = float[](0.22702703, 0.31621622, 0.07027027);
const float offsets[3] = float[](0.0, 1.38461538, 3.23076923);

vec3 brightPass(vec3 color) {
	const float threshold = push.scaleStrengthDirection.w;
	const float brightness = max(max(color.r, color.g), color.b);
	// soft knee; with threshold == 0.0 it behaves as an identity
	const float knee = max(threshold * 0.5f, 0.001f);
	return color * smoothstep(threshold, threshold + knee, brightness);
}

void main() {
	vec2 tex_offset = (1.0 / vec2(textureSize(u_texture, 0))) * push.scaleStrengthDirection.x;
	vec2 dir = (push.scaleStrengthDirection.z > 0.5) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
	vec2 step_uv = tex_offset * dir;

	// center sample
	vec3 result = texture(u_texture, v_uv).rgb * weights[0];
	const float threshold = push.scaleStrengthDirection.w;

	// two iterations (2 samples in each direction = 4 samples)
	for (int i = 1; i < 3; ++i) {
		vec2 offset = step_uv * offsets[i];
		result += brightPass(texture(u_texture, v_uv + offset).rgb) * weights[i];
		result += brightPass(texture(u_texture, v_uv - offset).rgb) * weights[i];
	}

	float strength = (push.scaleStrengthDirection.z > 0.5) ? 1.0 : push.scaleStrengthDirection.y;
	out_color = vec4(max(result, vec3(0.0)) * strength, 1.0);
}