#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_tangent;
//layout (location = 3) in uint a_sbo_offset; // per instance value

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_normal;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

// storage buffer
layout (set = 1, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout (set = 1, binding = 1) readonly buffer offsetIds {
   uint data[];
} in_offset_ids;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	v_uv = a_uv;

	{ // with 48 bytes transforms
		// common 
		const mat3x4 transform = sbo.transforms[in_offset_ids.data[gl_InstanceIndex + push.offset.x]];//sbo.transforms[a_sbo_offset];
		const mat4 model = mat4(
			transform[0],
			transform[1],
			transform[2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		const vec4 worldPosition = vec4(a_position, 1.0) * model;
		gl_Position = u_viewParams.viewProjection * worldPosition;
		
		const mat3 normalMatrix = transpose(inverse(mat3(u_viewParams.view)));

		const vec3 normal = normalize((vec4(a_normal, 0.0f) * model).xyz);
		v_normal = normalMatrix * normal;
	}
}