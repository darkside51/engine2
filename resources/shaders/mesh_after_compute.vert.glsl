#version 450

layout (location = 0) in vec2 a_uv;
layout (location = 1) in uint a_skeleton_ids; // per instance value

layout (set = 0, binding = 0) buffer Positions {
   vec4 data[];
} in_positions;

layout (set = 0, binding = 1) buffer Normals {
   vec4 data[];
} in_normals;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;

layout(push_constant) uniform Params {
	mat4 viewProjection;
	vec4 albedo;
	uint vertexPerObjects;
	uint vertexDataOffset;
} push;

void main() {
	//uint vertexId = push.vertexDataOffset + gl_VertexIndex + gl_InstanceIndex * push.vertexPerObjects;
	const uint vertexId = gl_VertexIndex + a_skeleton_ids * 10000;
	gl_Position = push.viewProjection * in_positions.data[vertexId];
	v_normal = in_normals.data[vertexId].xyz;
	v_uv = a_uv;
}