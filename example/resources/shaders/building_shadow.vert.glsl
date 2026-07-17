#version 450

#extension GL_ARB_shader_viewport_layer_array : enable

layout (constant_id = 0) const uint shadowCascades = 2;
layout (location = 0) in ivec4 a_position;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 cascades[shadowCascades];
} u_viewParams;

/*layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;*/

void main() {
	vec3 position = vec3(a_position.xyz) / 4096.0f;
	position *= 16.0f;
	position.y += 10.0f;
	position.z += 0.001f;
	vec4 worldPosition = vec4(position, 1.0);
	//gl_Position = u_viewParams.viewProjection * worldPosition;
	const uint count = 1;
	gl_Position = u_viewParams.cascades[gl_InstanceIndex / count] * worldPosition;
	gl_Layer = int(gl_InstanceIndex / count);
}