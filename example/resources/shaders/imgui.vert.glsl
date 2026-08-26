#version 450

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;

layout (set = 0, binding = 0) uniform UniformBuffer {
	vec4 transform;
} ubo;

void main()  {
	gl_Position = vec4(a_position * ubo.transform.xy + ubo.transform.zw, 0.0f, 1.0f);
	v_uv = a_uv;
	v_color = a_color;
}