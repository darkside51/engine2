#version 450

layout (location = 0) in vec3 a_position;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
	float m;
	float time;
} ubo;

void main() {
	gl_Position = ubo.model * vec4(0.2 * a_position.xy, 0.2 * -a_position.z, 1.0);
}