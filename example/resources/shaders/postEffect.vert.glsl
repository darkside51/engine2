#version 450

#include "common"

layout (location = 0) in vec2 a_position;
layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_rayDirWorld;
layout (location = 2) out vec3 v_rayDirView;

layout(push_constant) uniform Params {
	mat4 view;
	vec4 ambient;
	vec4 directionalLight;
	vec4 light;
	vec4 options;
} push;

void main() {
	gl_Position = vec4(a_position.xy, 0.0f, 1.0f);
	v_uv = (a_position.xy + 1.0f) * 0.5f;

	const mat3 m = transpose(mat3(
		push.view[0].xyz,
		push.view[1].xyz,
		push.view[2].xyz
	));

	v_rayDirWorld = createRay(gl_Position, m[1], -m[0], -m[2], push.view[1][3], push.view[0][3]);
	v_rayDirView = createRay(gl_Position, vec3(0.0f, 1.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), push.view[1][3], push.view[0][3]);
}