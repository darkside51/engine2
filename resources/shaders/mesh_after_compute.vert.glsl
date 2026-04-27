#version 450

layout (location = 0) in vec2 a_uv;
layout (location = 1) in uint a_skeleton_ids; // per instance value

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
} u_viewParams;

layout (set = 1, binding = 0) readonly buffer Positions {
   vec4 data[];
} in_positions;

layout (set = 1, binding = 1) readonly buffer Normals {
   vec4 data[];
} in_normals;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out vec4 v_color;
layout (location = 3) out vec3 v_emissive;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
} push;

void main() {
	//uint vertexId = push.vertexDataOffset + gl_VertexIndex + gl_InstanceIndex * push.vertexPerObjects;

	//const uint skeletonId = a_skeleton_ids & 0xffffu;
	//const uint vertexCount = (a_skeleton_ids >> 16u) & 0xffffu;
	//const uint vertexId = gl_VertexIndex + skeletonId * vertexCount;

	const uint vertexId = gl_VertexIndex + a_skeleton_ids;
	gl_Position = u_viewParams.viewProjection * in_positions.data[vertexId];
	v_normal = in_normals.data[vertexId].xyz;
	v_uv = a_uv;
	v_color = push.color;
	v_emissive = push.emissive.xyz;
}