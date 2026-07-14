#version 450

layout (location = 0) in ivec4 a_position;

layout (location = 0) out vec3 v_position;
layout (location = 1) out mat3 v_normalMatrix;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

void main() {
	v_position = vec3(a_position.xyz) / 4096.0f;
	v_position *= 16.0f;
	v_position.y += 10.0f;
	vec4 worldPosition = vec4(v_position, 1.0);
	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
}