#version 450

#include "common"


layout (location = 0) in vec3 v_worldPosition;

layout (location = 0) out float out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

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
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	vec3 normal = u_viewParams.viewTransposeInverse * -normalize(cross(dFdx(v_worldPosition.xyz), dFdy(v_worldPosition.xyz)));
	out_normal = vec4(normalize(normal) * 0.5 + 0.5, 0.0f);
	out_albedo = vec4(0.0, 1.0, 0.0, 1.0);
	out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}