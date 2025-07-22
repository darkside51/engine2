#version 450

layout (local_size_x = 1) in;

// storage buffer
layout (set = 0, binding = 0, std140) buffer SomeData {
   vec4 colors[ ];
} sbo; 

void main() {
	//uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;
	uint idx = gl_GlobalInvocationID.x;
	sbo.colors[idx] = vec4(1.0, 0.0, 0.0, 1.0);
}