#version 450

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 viewProjection;
} u_viewParams;

layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;

layout (set = 1, binding = 1) readonly buffer SkeletonIds {
   uint data[];
} in_color_ids;

layout(push_constant) uniform Params {
	uvec4 offset_and_count;
} push;

void main() {
	const uint offset = push.offset_and_count.x;
	const int vertexId = int(gl_VertexIndex + in_color_ids.data[gl_InstanceIndex + offset]);
	gl_Position = u_viewParams.viewProjection * imageLoad(in_positions, vertexId);
}