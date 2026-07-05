#version 450

#include "common"

layout (location = 0) in vec2 a_position;
layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_rayDir;

layout(push_constant) uniform Params {
	mat4 projection;
	vec4 options;
} push;


void main() {
	gl_Position = vec4(a_position.xy, 0.0, 1.0);
	v_uv = (a_position.xy + 1.0) * 0.5;

	const mat4 invProjection = inverse(push.projection);
	v_rayDir = createRay(gl_Position, vec3(0.0, 1.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(0.0, 0.0, -1.0), invProjection[1][1], invProjection[0][0]);
}