#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uvec4 a_joints;
layout (location = 4) in vec4 a_weights;
layout (location = 5) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec4 v_color;
layout (location = 3) out vec3 v_emissive;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

// storage buffer
layout (set = 1, binding = 0) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
} push;

void main() {
	v_uv = a_uv;
	v_color = push.color;
	v_emissive = push.emissive.xyz;

	{ // with 48 bytes transforms
		// skinned
		const mat3x4 skin3x4 = sbo_skin.transforms[a_sbo_offset + a_joints.x] * a_weights.x +
						  		     sbo_skin.transforms[a_sbo_offset + a_joints.y] * a_weights.y +
						  		     sbo_skin.transforms[a_sbo_offset + a_joints.z] * a_weights.z +
						  		     sbo_skin.transforms[a_sbo_offset + a_joints.w] * a_weights.w;

		const mat4 skin = mat4(
			skin3x4[0],
			skin3x4[1],
			skin3x4[2],
			vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);

		gl_Position = u_viewParams.viewProjection * (vec4(a_position, 1.0) * skin);
		v_normal = normalize((vec4(a_normal, 0.0) * skin).xyz);
	}
}