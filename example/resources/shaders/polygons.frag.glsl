#version 450

#include "common"
#include "uv"

layout (location = 0) in vec3 v_position;
layout (location = 1) flat in int v_material;

layout (location = 0) out float out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;
layout (location = 4) out vec2 out_metalicRoughness;

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

layout(set = 0, binding = 1) uniform sampler2D u_texture;
layout(set = 0, binding = 2) uniform samplerCube u_interriorMap;

struct Material {
	vec2 metalicRoughness;
	int imageId;
};

const Material materials[7] = Material[](
	Material(vec2(0.0f, 0.8f), 48),		// asphalt / default
	Material(vec2(0.0f, 0.55f), 57),	// grass
	Material(vec2(0.0f, 0.9f), 58),		// wood
	Material(vec2(0.0f, 0.75f), 59),	// playground
	Material(vec2(0.0f, 0.7f), 60),		// residential
	Material(vec2(0.0f, 0.8f), 61),		// parking
	Material(vec2(1.0f, 0.5f), 62)		// industrial
);

const float kImageSize = 128.0f;

vec4 getMaterialUv(ivec2 uv0) {
	const float textureSize = textureSize(u_texture, 0).x;
	return vec4(uv0 / textureSize, (uv0 + 1) / textureSize) * kImageSize;
}

vec4 getMaterialUv(int imageID) {
	const int size = textureSize(u_texture, 0).x / int(kImageSize);
	return getMaterialUv(ivec2(imageID % size, imageID / size));
}

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	const vec3 normal = normalize(cross(dFdx(v_position), -dFdy(v_position)));
	const vec3 viewNormal = u_viewParams.viewTransposeInverse * normal;

	out_normal = vec4(normalize(viewNormal) * 0.5f + 0.5f, 0.0f);

	const Material material = materials[v_material];

	const vec4 materialUvs = getMaterialUv(material.imageId);
	const float smoothFactor = 0.0f;//0.0125f;
	const vec2 uvMin = materialUvs.xy + vec2(smoothFactor);
	const vec2 uvMax = materialUvs.zw- vec2(smoothFactor);
	const vec2 size = uvMax - uvMin;

	vec2 uv = v_position.xy * size * 16.0f;
	const vec2 dx = dFdx(uv);
	const vec2 dy = dFdy(uv);

	const vec2 uv2 = repeatUVSimple(uv, uvMin, uvMax);
	const vec4 floorColor = textureGrad(u_texture, uv2, dx, dy);

	out_albedo = floorColor;
	out_metalicRoughness = vec2(0.0f, 1.0f);
	out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
}