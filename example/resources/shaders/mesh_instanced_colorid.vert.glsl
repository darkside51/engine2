#version 450

layout (location = 0) in vec3 a_position;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 viewProjection;
} u_viewParams;

// storage buffer
layout (set = 1, binding = 0) readonly buffer Transformations {
   mat3x4 transforms[];
} sbo;

layout (set = 1, binding = 1) readonly buffer offsetIds {
   uvec2 data[];
} in_color_ids;

layout(push_constant) uniform Params {
	vec2 renderOffset;
   uint offset;
   uint dummy;
} push;

layout (location = 0) flat out uint v_colorId;

void main() {
	{ // with 48 bytes transforms
		// common
		const uint offset = push.offset;
		uvec2 colorIds = in_color_ids.data[gl_InstanceIndex + offset];
		const mat3x4 transform = sbo.transforms[colorIds.x];
		const mat4 model = mat4(
			transform[0],
			transform[1],
			transform[2],
			vec4(0.0, 0.0, 0.0, 1.0)
		);

		vec4 worldPosition = vec4(a_position, 1.0) * model;
		gl_Position = u_viewParams.viewProjection * worldPosition;
		gl_Position.xy += push.renderOffset * gl_Position.w;
		v_colorId = colorIds.y;
	}
}