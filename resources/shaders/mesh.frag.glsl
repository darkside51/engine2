#version 450

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in float v_color_mix;

layout (location = 0) out vec4 out_color;

void main() {
	out_color = mix(vec4(v_uv, 0.0, 1.0), vec4(v_normal, 1.0), v_color_mix);
}