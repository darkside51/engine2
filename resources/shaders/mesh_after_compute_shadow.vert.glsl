#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 cascades[2];
} u_viewParams;

layout (set = 1, binding = 0) readonly buffer Positions {
   vec4 data[];
} in_positions;


layout (set = 1, binding = 1) readonly buffer SkeletonIds {
   uint data[];
} in_skeleton_ids;


layout(push_constant) uniform Params {
	uvec4 offset_and_count;
} push;

void main() {
	const uint offset = push.offset_and_count.x;
	const uint count = push.offset_and_count.y;
	const uint vertexId = gl_VertexIndex + in_skeleton_ids.data[gl_InstanceIndex % count + offset];
	gl_Position = u_viewParams.cascades[gl_InstanceIndex / count] * in_positions.data[vertexId];
	gl_Layer = int(gl_InstanceIndex / count);
}