#version 450

#include "common"

layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) in vec2 v_uv;
//layout (location = 1) in vec3 v_viewPosition;
layout (location = 1) in vec3 v_worldPosition;
layout (location = 2) in vec4 v_normalAndEmissive;
//layout (location = 3) in vec3 v_worldPosition;
layout (location = 3) in mat3 v_normalMatrix;

//layout (location = 0) out vec4 out_color;

layout (location = 0) out float out_position;
//layout (location = 1) out vec4 out_world_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

void main() {
	//out_color = texture(, v_uv);

	vec4 albedo = texture(texture0, v_uv);
	if (albedo.a < 0.5) {
		discard;
	}

	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	//out_position = vec4(v_viewPosition, depthLinear);
	//out_position = vec4(v_worldPosition, depthLinear);
	out_position = depthLinear;

	vec3 normal = v_normalMatrix * -normalize(cross(dFdx(v_normalAndEmissive.xyz), dFdy(v_normalAndEmissive.xyz)));

	out_normal = vec4(normalize(normal) * 0.5 + 0.5, 1.0);
	out_albedo = albedo;
	//out_albedo = texture(texture0, v_uv);
	//out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
	out_emissive = out_albedo * v_normalAndEmissive.w;
	//out_world_position = vec4(v_worldPosition, depthLinear);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}