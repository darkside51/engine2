#version 450

layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_color;
layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(texture0, v_uv) * vec4(v_color, 1.0);
}