#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 0) out vec2 v_uv;
//layout (location = 1) out vec3 v_color;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
	float m;
} ubo;

const mat3 identity = mat3(1.0, 0.0, 0.0,
					 	   0.0, 1.0, 0.0,
					 	   0.0, 0.0, 1.0);

const mat3 rotate_y90 = mat3(0.0, 0.0, -1.0,
					 	   0.0, 1.0, 0.0,
					 	   1.0, 0.0, 0.0);

const mat3 rotate_x90 = mat3(1.0, 0.0, 0.0,
					 	   0.0, 0.0, 1.0,
					 	   0.0, -1.0, 0.0);

const mat3 rotate_z90 = mat3(0.0, 1.0, 0.0,
					 	   -1.0, 0.0, 0.0,
					 	   0.0, 0.0, 1.0);

const mat3 rotate_y90z90 = rotate_y90 * rotate_z90;

void main() {
	const float radius = 1.0;
	const float xy = radius * cos(a_position.x); // r * cos(u)
    const float z = radius * sin(a_position.x);  // r * sin(u)
    const float x = xy * cos(a_position.y); // r * cos(u) * cos(v)
    const float y = xy * sin(a_position.y); // r * cos(u) * sin(v)

	// this is true!
	//vec3 sphereCoord = vec3(x, y, z) * rotate_y90z90;
	//vec3 plainCoord = vec3(a_position.y /** 0.5*/, a_position.x, 0.0);

	//vec3 sphereCoord = vec3(x, y, z);
	//vec3 plainCoord = vec3(a_position.x, a_position.y /** 0.5*/, 0.0) * rotate_y90;

	vec3 sphereCoord = vec3(x, y, z);
	vec3 plainCoord = vec3(-a_position.x, a_position.y, 0.0);

	gl_Position = ubo.model * mix(vec4(sphereCoord, 1.0), vec4(plainCoord, 1.0), ubo.m);
	//v_uv = vec2(16.0, 16.0) * a_uv;
	v_uv = a_uv;
	//v_uv.x -= ubo.model[1][1];

	//v_color = a_position;
	//gl_Position = ubo.model * vec4(a_position, 1.0);
}