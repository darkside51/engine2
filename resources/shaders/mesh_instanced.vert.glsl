#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;

layout (location = 0) out vec3 v_normal;
layout (location = 1) out vec2 v_uv;
layout (location = 2) out float v_color_mix;

// storage buffer
layout (set = 0, binding = 0, std140) buffer Transformations {
   mat4 transforms[ ];
} sbo;

layout(push_constant, std430) uniform InstanceOffsetData
{
    int offset;
} push;

void main() {
	gl_Position = sbo.transforms[push.offset + gl_InstanceIndex] * vec4(a_position, 1.0);

	v_normal = a_normal.rgb;
	v_uv = a_uv;
	v_color_mix = float(mod(gl_InstanceIndex, 2));
}