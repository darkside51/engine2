#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

layout (constant_id = 0) const uint shadowCascades = 2;
layout (location = 0) in ivec4 a_position;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 cascades[shadowCascades];
} u_viewParams;

layout(push_constant) uniform Params {
	mat4 transform;
} push;

void main() {
	vec3 position = vec3(a_position.xyz) / 4096.0f;
	vec4 worldPosition = push.transform * vec4(position, 1.0f);
	const uint count = 1;
	gl_Position = u_viewParams.cascades[gl_InstanceIndex / count] * worldPosition;
	gl_Layer = int(gl_InstanceIndex / count);
}