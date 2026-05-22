#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

layout (location = 0) in vec3 a_position;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 cascades[3];
} u_viewParams;

// storage buffer
layout (set = 1, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout (set = 1, binding = 1) readonly buffer offsetIds {
   uint data[];
} in_offset_ids;

layout(push_constant) uniform Params {
	uvec4 offset_and_count;
} push;

void main() {
	{ // with 48 bytes transforms
		// common
		const uint offset = push.offset_and_count.x;
		const uint count = push.offset_and_count.y;
		const mat3x4 transform = sbo.transforms[in_offset_ids.data[gl_InstanceIndex % count + offset]];
		const mat4 model = mat4(
			transform[0],
			transform[1],
			transform[2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		vec4 worldPosition = vec4(a_position, 1.0) * model;
		gl_Position = u_viewParams.cascades[gl_InstanceIndex / count] * worldPosition;
		gl_Layer = int(gl_InstanceIndex / count);
	}
}