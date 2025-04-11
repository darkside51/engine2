#version 450

//layout (set = 0, binding = 0) uniform UniformBuffer {
//	mat4 mvp;
//} ubo;

//layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) out vec4 out_color;

void main() {		
	out_color = vec4(1.0, 1.0, 1.0, 1.0);
}