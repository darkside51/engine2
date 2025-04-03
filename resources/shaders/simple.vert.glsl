#version 450

layout (location = 0) in vec3 a_position;

//layout(push_constant) uniform PushConstant {
//	mat4 mvp;
//} constants;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 mvp;
} ubo;

void main()  {
	gl_Position = ubo.mvp * vec4(a_position, 1.0);
}