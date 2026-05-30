#version 450

#import common

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
} u_viewParams;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec3 v_worldPosition;

layout(set = 2, binding = 0) uniform sampler2D u_texture;
layout(set = 2, binding = 1) uniform sampler2D u_texture_emissive;
layout(set = 2, binding = 2) uniform sampler2D u_texture_metalicRoughness;

layout (location = 0) out float out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	out_normal = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);
	out_albedo = texture(u_texture, v_uv) * push.color;
	out_emissive = vec4(texture(u_texture_emissive, v_uv).rgb * push.emissive.rgb, 1.0);
	out_metalicRoughness = vec2(texture(u_texture_metalicRoughness, v_uv).rg * push.metalicRoughness.xy);
}