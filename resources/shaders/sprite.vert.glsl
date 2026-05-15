#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec2 v_uv;
//layout (location = 1) out vec3 v_viewPosition;
layout (location = 1) out vec3 v_worldPosition;
layout (location = 2) out vec4 v_normalAndEmissive;
//layout (location = 3) out vec3 v_worldPosition;
layout (location = 3) out mat3 v_normalMatrix;

//layout(push_constant, std430) uniform Camera
//{
//	mat4 viewProjection;
//} push;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
} u_viewParams;

void main() {
	vec4 worldPosition = vec4(a_position, 1.0);
	gl_Position = u_viewParams.viewProjection * worldPosition;
	//v_viewPosition = vec3(u_viewParams.view * vec4(a_position, 1.0));
	v_worldPosition = worldPosition.xyz;
	v_uv = a_uv;
	v_normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
	v_normalAndEmissive = vec4(a_position, a_color.x);//v_normalMatrix * vec3(0.0, 0.0, 1.0);
}