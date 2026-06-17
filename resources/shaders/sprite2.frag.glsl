#version 450

#import common

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
} u_viewParams;

vec4 calculateUntexturedRoomsColour
	(
		vec3 position,			//object space
		vec3 wallFrequencies,
	    vec3 cameraPosition,	//object space
	    inout vec3 normal
	)
{
	mat3 m = mat3(-normal.y, normal.x, 0.0f,
				  -normal.x, -normal.y, 0.0f,
				  0.0f, 0.0f, 1.0f);
	// ????
    //position = position * m;
    //cameraPosition = cameraPosition * m;

	//get the vector from camera to surface
	vec3 direction = normalize(position - cameraPosition);

	//vec3 direction = normalize(position - cameraPosition) * m;
	//?????
	//cameraPosition.xyz = position - direction;
	//cameraPosition.y -= position.y - floor(position.y);

	//calculate wall locations
	vec3 walls = (floor(position * wallFrequencies) + step(vec3(0.0f), direction)) / wallFrequencies;

	//how much of the ray is needed to get from the cameraPosition to each of the walls
	vec3 rayFractions = (walls - cameraPosition) / direction;

	//texture-coordinates of intersections with walls
	vec2 intersectionX = 2.0f * (cameraPosition + rayFractions.x * direction).yz;
	vec2 intersectionY = 2.0f * (cameraPosition + rayFractions.y * direction).xz;
	vec2 intersectionZ = 2.0f * (cameraPosition + rayFractions.z * direction).xy;

	//choose normals for each of the three possible walls
	//(this depends on whether we are looking in positive or negative directions)
	vec3 signs = step(direction, vec3(0.0f)) * 2.0f - 1.0f;
	vec3 normalX = vec3(1.0f, 0.0f, 0.0f) * signs.x;
	vec3 normalY = vec3(0.0f, -1.0f, 0.0f) * signs.y;
	vec3 normalZ = vec3(0.0f, 0.0f, 1.0f) * signs.z;

	//choose the closest of these walls for the normal for the lighting
	float xVSy = step(rayFractions.x, rayFractions.y);
	float rayFraction_xVSy = mix(rayFractions.y, rayFractions.x, xVSy);
	vec2 intersection = mix(intersectionY, intersectionX, xVSy);

	vec3 interiorNormal = mix(normalY, normalX, xVSy);
	
	float xyVSz = step(rayFraction_xVSy, rayFractions.z);
	interiorNormal = mix(normalZ, interiorNormal, xyVSz);
	intersection = mix(intersectionZ, intersection, xyVSz);

	//return vec4(interiorNormal, 1.0);
	
	//float4 wallColour = tex2D(uniformTexture, 10 * intersection);

	float lightStrength = dot(interiorNormal, vec3(0.5f, 0.33166f, 0.f));
	vec4 interiorLight = clamp(lightStrength, 0.0f, 1.0f) * vec4(1.0f, 1.0f, 0.9f, 1.0f) + vec4(0.3f, 0.3f, 0.4f, 1.0f);
	
	normal = interiorNormal.xzy;

	return interiorLight * vec4(1.0f - texture(texture0, intersection).rgb, 1.0f);
}

vec4 calculateRoomCube(
	in samplerCube cubeMap,
	vec3 position,
	vec3 cameraPosition,
    vec3 normal,
    vec2 uv,
    vec3 dimensions,
    vec4 ambient,
    vec2 dir) {
	
	uv = fract(uv * 1.0f / vec2(dimensions.x, dimensions.z));
	const vec3 pos = vec3(uv.x * 2.0f - 1.0f, -1.0f, -uv.y * 2.0f + 1.0f);

	const mat3 m = mat3(-normal.y,  normal.x, 	0.0f,
				  		-normal.x, -normal.y, 	0.0f,
				  		0.0f, 		0.0f, 		1.0f);

	vec3 direction = (normalize(v_worldPosition - u_viewParams.cameraPosition) * m) / dimensions;
	const vec3 viewInv = 1.0f / (direction + vec3(0.00001f));

	const float fmin = min3(abs(viewInv) - viewInv * pos);
	direction = direction * fmin + pos;

	const vec3 absDir = abs(direction);
	const vec3 interiorNormal = normalize(vec3(
		step(absDir.y, absDir.x) * step(absDir.z, absDir.x),
		step(absDir.x, absDir.y) * step(absDir.z, absDir.y),
		step(absDir.x, absDir.z) * step(absDir.y, absDir.z)
	) * direction);
	//return vec4(interiorNormal, 1.0f);

	const float lightStrength = dot(interiorNormal, normalize(vec3(0.2f, 0.3f, -1.0f)));
	const vec4 interiorLight = clamp(lightStrength, 0.0f, 1.0f) + ambient;

	direction.xy = direction.xy * mat2(dir.x, dir.y, -dir.y, dir.x);
	return interiorLight * texture(cubeMap, normalize(direction));
}

void main() {
	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	out_position = depthLinear;

	vec3 normal = -normalize(cross(dFdx(v_normalAndEmissive.xyz), dFdy(v_normalAndEmissive.xyz)));
	
	vec2 fract_uv = fract(v_uv);

	vec4 window = texture(textureCubeMapArr, vec3(v_uv * vec2(1.0f, 2.0f) + vec2(0.0f, 0.5f), 3.0f));
	const float s = clamp((window.r + window.g + window.b) / 2.0f, 0.0f, 1.0f);

	//if (gl_FrontFacing && fract_uv.x > 0.35 && fract_uv.x < 0.7 && fract_uv.y > 0.35 && fract_uv.y < 0.7) {
	if (gl_FrontFacing && fract_uv.x > 0.47 && fract_uv.x < 0.97 && fract_uv.y > 0.26 && fract_uv.y < 0.75) {
		//const float bias = 0.00001f;
		//out_albedo = calculateUntexturedRoomsColour(v_worldPosition - normal * bias, vec3(1.0f, 1.0f, 1.0f), u_viewParams.cameraPosition, normal);

		// save wall directions:
		//const float cosa = dot(normal, vec3(0.0f, -1.0f, 0.0f));
		//out_albedo = calculateRoomCube(textureCubeMap, v_worldPosition, u_viewParams.cameraPosition, normal, v_uv, vec3(2.0f, 2.0f, 1.0f), vec4(1.0f), vec2(cosa, sqrt(1.0f - cosa*cosa)));
		//
		out_albedo = calculateRoomCube(textureCubeMap, v_worldPosition, u_viewParams.cameraPosition, normal, v_uv, vec3(2.0f, 1.5f, 1.0f), vec4(1.0f), vec2(1.0f, 0.0f));
		out_normal = vec4(normalize(v_normalMatrix * normal) * 0.5f + 0.5f, 1.0f);

		//vec3 direction = normalize(v_worldPosition - u_viewParams.cameraPosition);
		//out_albedo = mix(out_albedo, texture(textureCubeMap, -direction) * vec4(0.92f, 0.92f, 1.0f, 1.0f), 0.3f);
		//out_albedo = mix(out_albedo, vec4(0.67f, 0.82f, 1.0f, 1.0f), 0.3f);

		out_albedo = mix(out_albedo, 2.0f * window, s);
		out_metalicRoughness = vec2(0.7f, 0.3f);
	} else {
		const vec3 tangent = vec3(-1.0f, 0.0f, 0.0f);
		const vec3 binormal = (gl_FrontFacing ? 1.0f : -1.0f) * cross(normal, tangent) * sign(dot(normal, tangent));
		const mat3 tbn = mat3(v_normalMatrix * tangent, v_normalMatrix * binormal, v_normalMatrix * normal);

		vec2 uv = v_uv * 2.0f;
		out_albedo = texture(texture0, uv);
		out_metalicRoughness = vec2(0.0f, 1.0f);

		const vec3 tangentNormal = texture(texture_normal, uv).xyz * 2.0f - 1.0f;
		out_normal = vec4(normalize(tbn * tangentNormal) * 0.5f + 0.5f, 1.0f);
	}
	out_emissive = vec4(0.0f);
}