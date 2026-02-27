#version 450

layout (location = 0) in vec4 a_position;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;

layout(push_constant) uniform Camera {
	mat4 viewProjection;
	vec4 albedo;
} push;

void main() {
	gl_Position = push.viewProjection * a_position;
	v_normal = vec3(1.0);
	v_uv = vec2(0.0);
}