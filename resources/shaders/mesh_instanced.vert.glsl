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
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
} u_viewParams;

// storage buffer
layout (set = 1, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
} push;

void main() {
	v_uv = a_uv;
	v_color = push.color;
	v_emissive = push.emissive.xyz;

	{ // with 48 bytes transforms
		// common 
		const mat3x4 transform = sbo.transforms[a_sbo_offset];
		const mat4 model = mat4(
			transform[0],
			transform[1],
			transform[2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		gl_Position = u_viewParams.viewProjection * (vec4(a_position, 1.0) * model);
		v_normal = normalize((vec4(a_normal, 0.0) * model).xyz);
	}
}