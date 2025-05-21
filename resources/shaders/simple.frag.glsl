#version 450

//layout (set = 0, binding = 0) uniform UniformBuffer {
//	mat4 mvp;
//} ubo;

layout(set = 1, binding = 0) uniform sampler2D texture0;

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

void main() {
	out_color = v_color * texture(texture0, v_uv);
}