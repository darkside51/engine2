#version 450

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (set = 0, binding = 0) buffer positions {
   vec4 xyzw[];
} out_positions;

layout(push_constant, std430) uniform Transform {
	float time;
} push;

void main() {
	const uint vertexIndex = gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	out_positions.xyzw[vertexIndex].z = 0.4f * abs(sin(-200.0f * 6.28f * push.time + 2.0f * length(vec2(2.0f) - out_positions.xyzw[vertexIndex].xy)));
}