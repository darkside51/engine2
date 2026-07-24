#version 450

#include "common"
#include "interrior_maps"

layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_material;
layout (location = 2) in mat3 v_normalMatrix;
layout (location = 5) in vec2 v_w;

layout (location = 0) out float out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(set = 0, binding = 1) uniform sampler2D u_texture;
layout(set = 0, binding = 2) uniform sampler2D u_texture2;
layout(set = 0, binding = 3) uniform samplerCube u_interriorMap;
layout(set = 0, binding = 4) uniform sampler2DArray u_interriorMapArr;

const vec4 colors[6] = vec4[](
	vec4(0.65f, 0.65f, 0.65f, 1.0f), 	// building
	vec4(0.25f, 0.75f, 0.25f, 1.0f), 	// grass
	vec4(0.3f, 0.3f, 0.3f, 1.0f),		// pedestrian
	vec4(0.01f, 0.7f, 0.01f, 1.0f),		// wood
	vec4(0.45f, 0.45f, 0.45f, 1.0f),	// industrial
	vec4(0.2f, 0.35f, 0.95f, 1.0f)		// water
);

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	const vec3 normal = normalize(-cross(dFdx(v_position), dFdy(v_position)));
	const vec3 viewNormal = v_normalMatrix * normal;

	out_normal = vec4(normalize(viewNormal) * 0.5f + 0.5f, 0.0f);
	const vec4 color = colors[v_material] * (1.0f + v_position.z);
	if (v_material == 0u) {
		const float u = mix(v_position.x, v_position.y, step(abs(normal.y / normal.x), 1.0f));
		vec2 uv = vec2(u, -v_position.z) * 20.0f;
		vec2 uv2 = vec2(-v_w.x, -v_w.y);

		const vec2 fract_uv = fract(uv2);
		const vec2 lower = step(vec2(0.25f, 0.25f), fract_uv);
		const vec2 upper = step(fract_uv, vec2(0.75f, 0.75f));
		const float inside = lower.x * lower.y * upper.x * upper.y;

		const vec4 interriorColor = calculateRoomCube(u_interriorMap, v_position, u_viewParams.cameraPosition, normal, uv2, vec3(2.0f, 1.3f, 1.0f), vec4(1.0f));

		const vec4 window = texture(u_interriorMapArr, vec3(uv2 * vec2(1.0f, 2.0f) + vec2(-0.23f, 0.5f), 3.0f));
		const float s = clamp((window.r + window.g + window.b) / 2.0f, 0.0f, 1.0f);

		int m = int(round(v_position.x * 49.06f) * round(v_position.y * 49.06f)) % 6;
		//int m = (3000 * int(round(v_position.x * 4096.0f) * round(v_position.y * 4096.0f))) % 6;

		const vec4 wallColor = mix(texture(u_texture, uv) * colors[m], mix(interriorColor, window, s), inside);

		const vec4 floorColor = mix(texture(u_texture2, v_position.xy * 16.0f), color, step(0.01f, v_position.z));
		out_albedo = mix(wallColor, floorColor, step(0.99f, normal.z));
	} else {
		out_albedo = color;
	}
	out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
	out_metalicRoughness = vec2(0.0f, 1.0f);
}