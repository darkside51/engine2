#version 450

layout (location = 0) in ivec4 a_position;

layout (location = 0) out vec3 v_position;
layout (location = 1) flat out int v_material;
layout (location = 2) out mat3 v_normalMatrix;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(push_constant) uniform Params {
	mat4 transform;
} push;

void main() {
	v_position = vec3(a_position.xyz) / 4096.0f;
	vec4 worldPosition = push.transform * vec4(v_position, 1.0f);
	v_position = worldPosition.xyz / worldPosition.w;

	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
	v_material = a_position.w;
}