#version 450

#include "common"

//layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_material;
layout (location = 2) in mat3 v_normalMatrix;

layout (location = 0) out float out_position;
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

const vec4 colors[6] = vec4[](
	    vec4(0.65f, 0.65f, 0.65f, 1.0f), 	// building
	    vec4(0.25f, 0.75f, 0.25f, 1.0f), 	// grass
	    vec4(0.3f, 0.3f, 0.3f, 1.0f),		// pedestrian
	    vec4(0.01f, 0.7f, 0.01f, 1.0f),		// wood
	    vec4(0.45f, 0.45f, 0.45f, 1.0f),	// industrial
	    vec4(0.2f, 0.35f, 0.95f, 1.0f)		// water
	    );

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	vec3 normal = v_normalMatrix * -normalize(cross(dFdx(v_position), dFdy(v_position)));

	out_normal = vec4(normalize(normal) * 0.5 + 0.5, 0.0f);
	out_albedo = colors[v_material];
	out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}