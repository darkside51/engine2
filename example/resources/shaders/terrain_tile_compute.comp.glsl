#version 450

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0, r16f) writeonly uniform image2D heights;

layout(push_constant, std430) uniform Transform {
	float time;
} push;

void main() {
	const ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
	const uint vertexIndex = gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x);
	const float z = 2.0f * sin(-600.0f * 6.28f * push.time + 0.3f * length(vec2(16.0f) - texCoord));
	imageStore(heights, texCoord, vec4(z, 0.0, 0.0, 0.0));
}