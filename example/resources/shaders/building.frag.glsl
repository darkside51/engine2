#version 450

#include "common"
#include "uv"
#include "interrior_maps"

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec4 v_color;
layout (location = 2) flat in int v_material;
layout (location = 3) in vec2 v_w;

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

//layout(push_constant) uniform Params {
//	mat4 transform;
//	vec4 extent;
//} push;

layout(set = 0, binding = 1) uniform sampler2D u_texture;
layout(set = 0, binding = 2) uniform sampler2D u_texture2;
layout(set = 0, binding = 3) uniform sampler2D u_texture3;
layout(set = 0, binding = 4) uniform samplerCube u_interriorMap;
layout(set = 0, binding = 5) uniform sampler2DArray u_interriorMapArr;

struct Material {
	vec2 metalicRoughness;
	int imageId;
	int windowImageId;
};

const Material materials[11] = Material[](
	Material(vec2(0.0f, 1.0f), 4, 5),		// plaster / default
	Material(vec2(0.25f, 0.25f), 3, 2),		// glass
	Material(vec2(0.0f, 1.0f), 6, 7),		// wood
	Material(vec2(0.25f, 0.25f), 3, 5), 	// metal_plates / metal
	Material(vec2(0.1f, 0.4f), 9, 5), 		// granite
	Material(vec2(0.0f, 0.8f), 8, 5), 		// marble / vinyl
	Material(vec2(0.0f, 1.0f), 0, 5), 		// brick - limestone
	Material(vec2(0.0f, 1.0f), 1, 5), 		// cement_block / concrate / concrete
	Material(vec2(0.1f, 0.9f), 3, 5),		// porcelain_stoneware
	Material(vec2(0.0f, 1.0f), 10, 7),		// free!
	Material(vec2(0.0f, 1.0f), 11, 5) 		// brick
);

const vec4 colors[6] = vec4[](
	vec4(0.65f, 0.65f, 0.65f, 1.0f), 	// building
	vec4(0.25f, 0.75f, 0.25f, 1.0f), 	// grass
	vec4(0.9f, 0.9f, 0.1f, 1.0f),		// pedestrian
	vec4(0.01f, 0.7f, 0.01f, 1.0f),		// wood
	vec4(0.95f, 0.05f, 0.05f, 1.0f),	// industrial
	vec4(0.0f, 0.41f, 0.58f, 1.0f)		// water
);

const float kWallImageSize = 128.0f;

vec4 getMaterialUv(ivec2 uv0) {
	const float textureSize = textureSize(u_texture, 0).x;
	return vec4(uv0 / textureSize, (uv0 + 1) / textureSize) * kWallImageSize;
}

vec4 getMaterialUv(int imageID) {
	const int size = textureSize(u_texture, 0).x / int(kWallImageSize);
	return getMaterialUv(ivec2(imageID % size, imageID / size));
}

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	const vec3 normal = normalize(-cross(dFdx(v_position), dFdy(v_position)));
	const vec3 viewNormal = u_viewParams.viewTransposeInverse * normal;

	out_normal = vec4(normalize(viewNormal) * 0.5f + 0.5f, 0.0f);
	if (v_material < 11) {
		const float u = mix(v_position.x, v_position.y, step(abs(normal.y / normal.x), 1.0f));

		vec2 uv = vec2(u * 16.0f, -v_w.y);
		vec2 uv2 = vec2(-v_w.x, -v_w.y);

		const vec4 interriorColor = calculateRoomCube(u_interriorMap, v_position, u_viewParams.cameraPosition, normal, uv2, vec3(2.0f, 1.0f, 1.0f), vec4(1.0f));

		const float kUvMix = 0.0001f;

		//vec2 uvMin = mix(vec2(0.0f, 0.0f), vec2(0.5f, 0.0f), v_material & 1);
		//vec2 uvMax = mix(vec2(0.5f, 0.5f), vec2(1.0f, 0.5f), v_material & 1);

		const Material material = materials[v_material];

		const vec4 materialUvs = getMaterialUv(material.imageId);
		const vec2 uvMin = materialUvs.xy;
		const vec2 uvMax = materialUvs.zw;
		const vec2 size = uvMax - uvMin;

		uv = repeatUVSimple(uv * size, uvMin, uvMax);
		vec2 uv3 = uv2 * size;
		const vec2 dx = dFdx(uv3);
		const vec2 dy = dFdy(uv3);

		const vec4 windowMaterialUvs = getMaterialUv(material.windowImageId);
		const vec2 uvWindowMin = windowMaterialUvs.xy + vec2(0.005f, 0.005f);
		const vec2 uvWindowMax = windowMaterialUvs.zw - vec2(0.005f, 0.005f);
		const vec2 size2 = uvWindowMax - uvWindowMin;
		const vec2 windowUv = repeatUVSimple(uv2 * size2, uvWindowMin, uvWindowMax);

		uv3 = repeatUVSimple(uv3, uvMin, uvMax);

		const float magicStep = step(abs(dx.x), kUvMix);

		vec4 wallMaterialColor = textureGrad(u_texture, mix(uv3, uv, magicStep), dx, dy);
		wallMaterialColor.rgb *= v_color.rgb;

		const vec4 window = mix(textureGrad(u_texture, windowUv, dx, dy), vec4(0.0f), magicStep);
		wallMaterialColor = mix(wallMaterialColor, window, step(0.3f, window.a));
		const vec4 wallColor = mix(wallMaterialColor, interriorColor, 1.0f - wallMaterialColor.a);

		const vec4 color = texture(u_texture2, v_position.xy * 8.0f) * (1.0f + v_position.z) * v_color;
		const vec4 floorColor = mix(texture(u_texture3, v_position.xy * 10.0f), color, step(0.01f, v_position.z));
		const float roof = step(0.99f, abs(normal.z));
		
		out_albedo = mix(wallColor, floorColor, roof);
		out_metalicRoughness = mix(material.metalicRoughness, vec2(0.0f, 0.1f), step(wallMaterialColor.a, 0.95f) * (1.0f - roof));
	} else {
		const vec4 color = /*colors[v_material - 4] * */vec4(1.0f) * (1.0f + v_position.z);
		out_albedo = color * v_color;
		out_metalicRoughness = vec2(0.0f, 1.0f);
	}
	
	out_emissive = vec4(0.0, 0.0, 0.0, 0.0);
}