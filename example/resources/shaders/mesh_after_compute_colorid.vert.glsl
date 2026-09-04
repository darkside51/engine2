#version 450

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 viewProjection;
} u_viewParams;

layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;

layout (set = 1, binding = 1) readonly buffer SkeletonIds {
   uvec2 data[];
} in_color_ids;

layout(push_constant) uniform Params {
	uvec4 offset_and_count;
} push;

layout (location = 0) flat out uint v_colorId;

void main() {
	const uint offset = push.offset_and_count.x;
	uvec2 colorIds = in_color_ids.data[gl_InstanceIndex + offset];
	const int vertexId = int(gl_VertexIndex + colorIds.x);
	gl_Position = u_viewParams.viewProjection * imageLoad(in_positions, vertexId);
	v_colorId = colorIds.y;
}