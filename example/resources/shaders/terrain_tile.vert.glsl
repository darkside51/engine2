#version 450

layout (location = 0) in vec4 a_position;
layout (location = 0) out vec3 v_worldPosition;

layout(push_constant, std430) uniform Transform {
	mat4 model;
} push;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	vec2 cursorPosition;
	float time;
} u_viewParams;

void main() {
	vec4 worldPosition = push.model * a_position;
	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_worldPosition = worldPosition.xyz / worldPosition.w;
}