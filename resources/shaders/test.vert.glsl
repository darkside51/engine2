#version 450

layout (location = 0) in vec2 a_position;
layout (location = 0) out vec2 v_uv;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
} ubo;

void main() {
	gl_Position = ubo.model * vec4(a_position.xy * 0.5, 0.0, 1.0);
	v_uv = (a_position.xy + 1.0) * 0.5;
}