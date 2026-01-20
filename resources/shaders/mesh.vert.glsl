#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 v_uv;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
	float m;
	float time;
} ubo;

void main() {
	vec2 offset = vec2(-16.0 + mod(gl_InstanceIndex, 64) * 0.5, -16.0 + (gl_InstanceIndex / 64) * 0.5);
	gl_Position = ubo.model * vec4(0.2 * a_position.xy + offset, 0.2 * -a_position.z, 1.0);
	v_uv = a_uv;
}