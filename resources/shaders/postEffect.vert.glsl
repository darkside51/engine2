#version 450

#import common

layout (location = 0) in vec2 a_position;
layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_rayDir;

layout(push_constant) uniform Params {
	mat4 view;
	vec4 ambient;
	vec4 directionalLight;
	vec4 light;
	vec4 options;
} push;

void main() {
	gl_Position = vec4(a_position.xy, 0.0, 1.0);
	v_uv = (a_position.xy + 1.0) * 0.5;

	const mat3 m = transpose(mat3(
		push.view[0].xyz,
		push.view[1].xyz,
		push.view[2].xyz
	));

	v_rayDir = createRay(gl_Position, m[1], -m[0], -m[2], push.view[1][3], push.view[0][3]);
}