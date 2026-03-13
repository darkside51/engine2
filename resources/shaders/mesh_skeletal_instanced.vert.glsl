#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uvec4 a_joints;
layout (location = 4) in vec4 a_weights;
layout (location = 5) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;

// storage buffer
layout (set = 0, binding = 0) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

layout(push_constant) uniform Camera {
	mat4 viewProjection;
	vec4 albedo;
} push;

void main() {
	v_uv = a_uv;

	{ // with 48 bytes transforms
		// skinned
		const mat3x4 transforms[] = mat3x4[](
			sbo_skin.transforms[a_sbo_offset + a_joints.x],
			sbo_skin.transforms[a_sbo_offset + a_joints.y],
			sbo_skin.transforms[a_sbo_offset + a_joints.z],
			sbo_skin.transforms[a_sbo_offset + a_joints.w]
		);

		const mat4 skin = mat4(
			transforms[0][0],
			transforms[0][1],
			transforms[0][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.x +
		mat4(
			transforms[1][0],
			transforms[1][1],
			transforms[1][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.y +
		mat4(
			transforms[2][0],
			transforms[2][1],
			transforms[2][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.z +
		mat4(
			transforms[3][0],
			transforms[3][1],
			transforms[3][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.w;

		gl_Position = push.viewProjection * (vec4(a_position, 1.0) * skin);
		v_normal = normalize((vec4(a_normal, 0.0) * skin).xyz);
	}
}