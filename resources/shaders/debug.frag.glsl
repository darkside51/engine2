#version 450

layout (location = 0) in vec4 v_color;
//layout (location = 0) out vec4 o_color;

layout (location = 0) out float out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

void main() {
	out_position = 1.0f;
	out_normal = vec4(0.0f);
	out_albedo = v_color;
	out_emissive = vec4(0.0f);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}