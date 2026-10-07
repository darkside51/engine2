#version 450

#include "common"

layout (location = 0) in vec2 v_tileUv;
layout (location = 1) in vec2 v_globalUv;

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


layout(set = 0, binding = 2) uniform sampler2D u_colors;
layout(set = 0, binding = 3) uniform sampler2D u_normals;

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	const vec2 gtexSize = vec2(textureSize(u_normals, 0));
	const vec3 normal = normalize(u_viewParams.viewTransposeInverse * -texture(u_normals, v_globalUv / gtexSize).xyz);
	
	out_normal = vec4(normal * 0.5f + 0.5f, 0.0f);
	out_albedo = texture(u_colors, v_tileUv);
	out_emissive = vec4(0.0f, 0.0f, 0.0f, 0.f);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}