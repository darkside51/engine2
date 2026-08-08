#version 450

layout (location = 0) in ivec4 a_position;

layout (location = 0) out vec3 v_position;
layout (location = 1) flat out int v_material;
layout (location = 2) out vec2 v_w;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(push_constant) uniform Params {
	mat4 transform;
	vec4 extent;
} push;

void main() {
	v_position = vec3(a_position.xyz) / push.extent.x;
	vec4 worldPosition = push.transform * vec4(v_position, 1.0f);
	v_position = worldPosition.xyz / worldPosition.w;

	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_material = a_position.w & 0x000f;
	v_w = vec2(float(a_position.w >> 4), round(v_position.z * 16.0f));
}