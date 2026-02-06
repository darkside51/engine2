#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uvec4 a_joints;
layout (location = 4) in vec4 a_weights;
layout (location = 5) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out float v_color_mix;

// storage buffer
layout (set = 0, binding = 0) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

layout(push_constant) uniform Camera {
	mat4 viewProjection;
} push;

void main() {
	v_uv = a_uv;
	v_color_mix = 0.0;

	{ // with 48 bytes transforms
		// skinned
		const mat4 skin = mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.x][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.x][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.x][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.x +
		mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.y][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.y][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.y][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.y +
		mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.z][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.z][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.z][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.z +
		mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.w][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.w][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.w][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.w;

		gl_Position = push.viewProjection * (vec4(a_position, 1.0) * skin);
		v_normal = normalize((vec4(a_normal, 0.0) * skin).xyz);
	}
}