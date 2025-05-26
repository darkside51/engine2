#version 450

layout (location = 0) in vec3 a_position;


layout (set = 0, binding = 0) uniform UniformBuffer {
	vec2 offset;
} ubo;

layout (set = 0, binding = 1) uniform UniformBuffer2 {
	vec2 offset;
} ubo2;


void main()  {
	gl_Position = vec4(a_position.xy + ubo.offset + ubo2.offset, a_position.z, 1.0);
}