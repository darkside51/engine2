#version 450

layout (location = 0) in vec2 a_position;

layout (location = 0) out vec2 v_tileUv;
layout (location = 1) out vec2 v_globalUv;

layout(push_constant, std430) uniform Transform {
	mat4 model;
	uint instanceOffset;
	uint tileSize;
	uint width;
} push;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	vec2 cursorPosition;
	float time;
} u_viewParams;

layout (set = 0, binding = 1) uniform usamplerBuffer u_indexTexelBuffer;

//layout(set = 0, binding = 2, r16f) readonly uniform image2D heights;

void main() {
	const uint texel = texelFetch(u_indexTexelBuffer, int(gl_InstanceIndex + push.instanceOffset)).r;
	const float y = float(texel / push.width) * push.tileSize;
	const float x = float(texel & (push.width - 1u)) * push.tileSize; // texel % w for pow2

 	const vec4 heightTexel = vec4(0.0f);//imageLoad(heights, ivec2(a_position.xy));
	vec4 worldPosition = push.model * vec4(a_position.xy + vec2(x, y), heightTexel.x * push.tileSize, 1.0f);
	gl_Position = u_viewParams.viewProjection * worldPosition;
	v_tileUv = a_position.xy / push.tileSize;
	v_globalUv = a_position.xy + vec2(x, y);
}