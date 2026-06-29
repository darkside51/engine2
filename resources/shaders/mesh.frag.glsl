#version 450

#include "common"

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughnessAlphaCut;
	uvec4 offset;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_worldPosition;
layout (location = 2) in mat3 v_tbn;

layout(set = 2, binding = 0) uniform sampler2D u_texture_color;
layout(set = 2, binding = 1) uniform sampler2D u_texture_emissive;
layout(set = 2, binding = 2) uniform sampler2D u_texture_metalicRoughness;
layout(set = 2, binding = 3) uniform sampler2D u_texture_normal;

layout (location = 0) out float out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

void main() {
	out_albedo = texture(u_texture_color, v_uv) * push.color;

	//if (gl_FragCoord.x > 100.0f && gl_FragCoord.x < 500.0f) {
	//	out_albedo *= vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//}

	if (out_albedo.a < push.metalicRoughnessAlphaCut.z) {
		discard;
		return;
	}
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	vec3 tangentNormal = texture(u_texture_normal, v_uv).xyz * 2.0f - 1.0f;
	out_normal = normalize(v_tbn * tangentNormal) * 0.5f + 0.5f;
	out_emissive = vec4(texture(u_texture_emissive, v_uv).rgb * push.emissive.rgb, 1.0f);
	out_metalicRoughness = vec2(texture(u_texture_metalicRoughness, v_uv).bg * push.metalicRoughnessAlphaCut.xy);
}