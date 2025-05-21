#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_color;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;

//layout(push_constant) uniform PushConstant {
//	mat4 mvp;
//} constants;

layout (set = 0, binding = 0) uniform UniformBuffer {
	vec2 offset;
} ubo;

layout (set = 0, binding = 1) uniform UniformBuffer2 {
	vec2 offset;
} ubo2;


void main()  {
	//gl_Position = ubo.mvp * vec4(a_position, 1.0);
	gl_Position = vec4(a_position.xy + ubo.offset + ubo2.offset, a_position.z, 1.0);
	//gl_Position = vec4(a_position, 1.0);
	v_color = a_color;
	v_uv = (a_position.xy + 1.0) * 0.5;
}