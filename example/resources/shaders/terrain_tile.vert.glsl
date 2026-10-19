#version 450

layout (location = 0) in vec2 a_position;
layout (location = 0) out vec3 v_worldPosition;
layout (location = 1) out vec2 v_uv;

layout(push_constant, std430) uniform Transform {
	mat4 model;
	float cellSize;
	float uvCellSize;
} push;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(set = 0, binding = 1, r16f) readonly uniform image2D heights;

void main() {
 	const vec4 heightTexel = imageLoad(heights, ivec2(a_position.xy));
	vec4 worldPosition = push.model * vec4(a_position.xy * push.cellSize, heightTexel.x * push.cellSize, 1.0f);
	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_worldPosition = worldPosition.xyz / worldPosition.w;
	v_uv = a_position.xy / 31.0f;
}