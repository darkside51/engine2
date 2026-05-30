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

layout (set = 1, binding = 0) readonly buffer Positions {
   vec4 data[];
} in_positions;

layout (set = 1, binding = 1) readonly buffer Normals {
   vec4 data[];
} in_normals;

layout (set = 1, binding = 2) readonly buffer SkeletonIds {
   uint data[];
} in_skeleton_ids;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec3 v_worldPosition;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	const uint vertexId = gl_VertexIndex + in_skeleton_ids.data[gl_InstanceIndex + push.offset.x];//a_skeleton_ids;
	gl_Position = u_viewParams.viewProjection * in_positions.data[vertexId];
	v_normal = in_normals.data[vertexId].xyz;
	v_uv = a_uv;

	v_worldPosition = in_positions.data[vertexId].xyz;
	mat3 normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
	v_normal = normalMatrix * v_normal;
}