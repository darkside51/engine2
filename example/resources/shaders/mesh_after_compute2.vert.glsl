#version 450

layout (location = 0) in vec2 a_uv;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	vec2 cursorPosition;
	float time;
} u_viewParams;

layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;
layout(set = 1, binding = 1, rgba8_snorm) readonly uniform imageBuffer in_normals;
layout(set = 1, binding = 2, rgba8_snorm) readonly uniform imageBuffer in_tangents;

layout (set = 1, binding = 3) readonly buffer SkeletonIds {
   uvec2 data[];
} in_skeleton_ids;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_normal;
layout (location = 2) flat out uint v_nodeId;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	const uvec2 ids = in_skeleton_ids.data[gl_InstanceIndex + push.offset.x];
	const int vertexId = int(gl_VertexIndex + ids.x);//a_skeleton_ids;
	const vec4 position = imageLoad(in_positions, vertexId); // in_positions.data[vertexId];
	gl_Position = u_viewParams.viewProjection * position;
	v_uv = a_uv;
	
	const vec3 normal = imageLoad(in_normals, int(vertexId)).xyz;
	v_normal = u_viewParams.viewTransposeInverse * normal;
	v_nodeId = ids.y;
}