#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out float v_color_mix;

// storage buffer
layout (set = 0, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout(push_constant) uniform Camera {
	mat4 viewProjection;
} push;

void main() {
	v_uv = a_uv;
	v_color_mix = 0.0;

	{ // with 48 bytes transforms
		// common 
		const mat4 model = mat4(
			sbo.transforms[a_sbo_offset][0],
			sbo.transforms[a_sbo_offset][1],
			sbo.transforms[a_sbo_offset][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		gl_Position = push.viewProjection * (vec4(a_position, 1.0) * model);
		v_normal = (vec4(a_normal, 0.0) * model).xyz;
	}
}