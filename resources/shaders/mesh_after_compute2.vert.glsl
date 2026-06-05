#version 450

layout (location = 0) in vec2 a_uv;
//layout (location = 1) in uint a_skeleton_ids; // per instance value

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
} u_viewParams;

//layout (set = 1, binding = 0) readonly buffer Positions {
//   vec4 data[];
//} in_positions;

//layout (set = 1, binding = 1) readonly buffer Normals {
//   vec4 data[];
//} in_normals;

layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;
layout(set = 1, binding = 1, rgba8_snorm) readonly uniform imageBuffer in_normals;
layout(set = 1, binding = 3, rgba8_snorm) readonly uniform imageBuffer in_tangents;

layout (set = 1, binding = 2) readonly buffer SkeletonIds {
   uint data[];
} in_skeleton_ids;

//layout (set = 1, binding = 3) readonly buffer Tangents {
//   vec4 data[];
//} in_tangents;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec3 v_worldPosition;
layout (location = 2) out vec3 v_normal;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	const uint vertexId = gl_VertexIndex + in_skeleton_ids.data[gl_InstanceIndex + push.offset.x];//a_skeleton_ids;
	const vec4 position = imageLoad(in_positions, int(vertexId)); // in_positions.data[vertexId];
	gl_Position = u_viewParams.viewProjection * position;
	v_uv = a_uv;

	v_worldPosition = position.xyz;
	const mat3 normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
	
	const vec3 normal = imageLoad(in_normals, int(vertexId)).xyz;//in_normals.data[vertexId].xyz;
	v_normal = normalMatrix * normal;
}