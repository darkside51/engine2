#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec4 v_color;
layout (location = 3) out vec3 v_emissive;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 model;
	float m;
	float time;
} ubo;

void main() {
	vec2 offset = vec2(-16.0 + mod(gl_InstanceIndex, 64) * 0.5, -16.0 + (gl_InstanceIndex / 64) * 0.5);
	offset.x += float(a_sbo_offset); // remove it - useless
	gl_Position = ubo.model * vec4(0.15 * a_position.xy + offset, 0.15 * -a_position.z, 1.0);
	//v_normal = unpackUnorm4x8(a_normal).rgb;
	v_normal = a_normal.rgb;
	v_uv = a_uv;
	v_color = vec4(1.0);
	v_emissive = vec3(0.0);
	//v_color_mix = float(mod(gl_InstanceIndex + a_sbo_offset, 2));
}