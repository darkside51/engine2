#version 450

layout (set = 0, binding = 0) uniform sampler2D u_texture;

layout(push_constant) uniform Params {
	vec4 scaleStrengthDirection;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

const float weight[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
	vec2 tex_offset = 1.0 / textureSize(u_texture, 0) * push.scaleStrengthDirection.x; // gets size of single texel
	vec3 result = texture(u_texture, v_uv).rgb * weight[0]; // current fragment's contribution
	for(int i = 1; i < 5; ++i) {
		if (push.scaleStrengthDirection.z > 0.5f) {
			// H
			result += texture(u_texture, v_uv + vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * push.scaleStrengthDirection.y;
			result += texture(u_texture, v_uv - vec2(tex_offset.x * i, 0.0)).rgb * weight[i] * push.scaleStrengthDirection.y;
		} else {
			// V
			result += texture(u_texture, v_uv + vec2(0.0, tex_offset.y * i)).rgb * weight[i] * push.scaleStrengthDirection.y;
			result += texture(u_texture, v_uv - vec2(0.0, tex_offset.y * i)).rgb * weight[i] * push.scaleStrengthDirection.y;
		}
	}
	out_color = vec4(result, 1.0);
}