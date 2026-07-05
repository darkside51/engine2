#version 450

#include "common"
#include "interrior_maps"

layout(set = 0, binding = 1) uniform sampler2D texture0;
layout(set = 0, binding = 2) uniform sampler2D texture_normal;
layout(set = 0, binding = 3) uniform samplerCube textureCubeMap;
layout(set = 0, binding = 4) uniform sampler2DArray textureCubeMapArr;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_worldPosition;
layout (location = 2) in vec4 v_normalAndEmissive;
layout (location = 3) in mat3 v_normalMatrix;

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

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	vec3 normal = -normalize(cross(dFdx(v_normalAndEmissive.xyz), dFdy(v_normalAndEmissive.xyz)));
	
	vec2 fract_uv = fract(v_uv);

	vec4 window = texture(textureCubeMapArr, vec3(v_uv * vec2(1.0f, 2.0f) + vec2(0.0f, 0.5f), 3.0f));
	const float s = clamp((window.r + window.g + window.b) / 2.0f, 0.0f, 1.0f);

	if (gl_FrontFacing && fract_uv.x > 0.47 && fract_uv.x < 0.97 && fract_uv.y > 0.26 && fract_uv.y < 0.75) {
		//const float bias = 0.00001f;
		//out_albedo = calculateUntexturedRoomsColour(v_worldPosition - normal * bias, vec3(1.0f, 1.0f, 1.0f), u_viewParams.cameraPosition, normal);

		// save wall directions:
		//const float angle = -0.35f - 1.57f;
		//out_albedo = calculateRoomCube(textureCubeMap, v_worldPosition, u_viewParams.cameraPosition, normal, v_uv, vec3(2.0f, 2.0f, 1.0f), vec4(1.0f), vec3(cos(angle), sin(angle), 0.0f));
		//
		out_albedo = calculateRoomCube(textureCubeMap, v_worldPosition, u_viewParams.cameraPosition, normal, v_uv, vec3(2.0f, 1.25f, 1.0f), vec4(1.0f));
		out_normal = vec4(normalize(v_normalMatrix * normal) * 0.5f + 0.5f, 0.0f);

		//vec3 direction = normalize(v_worldPosition - u_viewParams.cameraPosition);
		//out_albedo = mix(out_albedo, texture(textureCubeMap, -direction) * vec4(0.92f, 0.92f, 1.0f, 1.0f), 0.3f);
		//out_albedo = mix(out_albedo, vec4(0.67f, 0.82f, 1.0f, 1.0f), 0.3f);

		out_albedo = mix(out_albedo, window, s);
		out_metalicRoughness = vec2(0.7f, 0.3f);

		//float rnd = noise2(20.0f * v_uv * 0.5f + 0.5f);
    	//out_albedo = vec4(vec3(rnd),1.0);
	} else {
		const vec3 tangent = vec3(-1.0f, 0.0f, 0.0f);
		const vec3 binormal = (gl_FrontFacing ? 1.0f : -1.0f) * cross(normal, tangent) * sign(dot(normal, tangent));
		const mat3 tbn = mat3(v_normalMatrix * tangent, v_normalMatrix * binormal, v_normalMatrix * normal);

		vec2 uv = v_uv * 2.0f;
		out_albedo = texture(texture0, uv);
		out_metalicRoughness = vec2(0.0f, 1.0f);

		const vec3 tangentNormal = texture(texture_normal, uv).xyz * 2.0f - 1.0f;
		out_normal = vec4(normalize(tbn * tangentNormal) * 0.5f + 0.5f, 0.0f);
	}
	out_emissive = vec4(0.0f);
}