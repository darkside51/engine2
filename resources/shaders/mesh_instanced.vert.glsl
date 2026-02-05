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
layout (set = 0, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout (set = 0, binding = 1) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

layout(push_constant) uniform Camera {
	mat4 viewProjection;
} push;

void main() {
	// gl_Position = push.viewProjection * (sbo.transforms[a_sbo_offset] * vec4(a_position, 1.0));
	v_uv = a_uv;
	v_color_mix = a_sbo_offset % 2; // float(mod(a_sbo_offset, 2));

	{ // with 48 bytes transforms
		/*
		// common 
		const mat4 modelTransposed = mat4(
			sbo.transforms[a_sbo_offset][0],
			sbo.transforms[a_sbo_offset][1],
			sbo.transforms[a_sbo_offset][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		gl_Position = push.viewProjection * (vec4(a_position, 1.0) * modelTransposed);
		v_normal = (vec4(a_normal, 0.0) * modelTransposed).xyz;
		*/

		// skinned
		const mat4 skin0 = mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.x][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.x][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.x][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.x;

		const mat4 skin1 = mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.y][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.y][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.y][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.y;

		const mat4 skin2 = mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.z][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.z][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.z][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.z;

		const mat4 skin3 = mat4(
			sbo_skin.transforms[a_sbo_offset + a_joints.w][0],
			sbo_skin.transforms[a_sbo_offset + a_joints.w][1],
			sbo_skin.transforms[a_sbo_offset + a_joints.w][2],
			vec4(0.0, 0.0, 0.0, 1.0)
		) * a_weights.w;

		const mat4 skin = skin0 + skin1 + skin2 + skin3;

		gl_Position = push.viewProjection * (vec4(a_position, 1.0) * skin);
		v_normal = (vec4(a_normal, 0.0) * skin).xyz;
		v_color_mix += sbo.transforms[a_sbo_offset][0].x; // we must use all sets in shader for correct working
	}
}