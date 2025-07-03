#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_color;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
} ubo;

void main() {
	gl_Position = ubo.model * vec4(a_position, 1.0);
	v_uv = a_uv;
	v_color = a_position;
}