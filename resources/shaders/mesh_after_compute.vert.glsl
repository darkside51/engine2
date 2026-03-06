#version 450

layout (location = 0) in vec2 a_uv;

layout (set = 0, binding = 0) buffer Positions {
   vec4 data[];
} in_positions;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;

layout(push_constant) uniform Params {
	mat4 viewProjection;
	vec4 albedo;
	uint vertexPerObjects;
	uint vertexDataOffset;
} push;

void main() {
	gl_Position = push.viewProjection * in_positions.data[push.vertexDataOffset + gl_VertexIndex + gl_InstanceIndex * push.vertexPerObjects];
	v_normal = vec3(1.0);
	v_uv = a_uv;
}