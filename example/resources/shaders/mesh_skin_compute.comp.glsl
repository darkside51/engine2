#version 450

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// Syntax: layout(set = X, binding = Y, format_qualifier) writeonly uniform imageBuffer variableName;
layout(set = 0, binding = 0, rgba32f) writeonly uniform imageBuffer out_positions;
layout(set = 0, binding = 1, rgba8_snorm) writeonly uniform imageBuffer out_normals;
layout(set = 0, binding = 2, rgba8_snorm) writeonly uniform imageBuffer out_tangents;

// storage buffers
layout (set = 1, binding = 0) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

layout (set = 1, binding = 1) readonly buffer SkinTransformOffsets {
   uint data[];
} skin_offsets;

layout (set = 1, binding = 2) readonly buffer SkeletonIds {
   uint data[];
} skeleton_ids;

layout (set = 2, binding = 0) readonly buffer UniqueIndices {
   uint data[];
} u_indices;

// uniform texel buffers
layout(set = 2, binding = 1) uniform textureBuffer u_positions;
layout(set = 2, binding = 2) uniform textureBuffer u_normals;
layout(set = 2, binding = 3) uniform textureBuffer u_joints;
layout(set = 2, binding = 4) uniform textureBuffer u_weights;
layout(set = 2, binding = 5) uniform textureBuffer u_tangents;

layout(push_constant) uniform Data {
	uint firstObject;
	uint objectsCount;
	uint invocationsPerObject;
	uint indicesOffset;

	uint transformsOffset;
	uint skeletonsOffset;
} push;

void main() {
	const uint invocationId = gl_GlobalInvocationID.x;

	//const uint invocationId = gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x) + (gl_GlobalInvocationID.z * 
	//	gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y);

	const uint objectId = invocationId / push.invocationsPerObject;

	if (objectId >= push.objectsCount) {
		return;
	}

	const uint transformIdx = skin_offsets.data[objectId + push.firstObject + push.transformsOffset];
	const int vertexId = int(u_indices.data[invocationId % push.invocationsPerObject + push.indicesOffset]);

	const vec4 a_weights = texelFetch(u_weights, vertexId);
	const uvec4 a_joints = uvec4(texelFetch(u_joints, vertexId) * 255.0f);

	const mat3x4 skin3x4 = sbo_skin.transforms[transformIdx + a_joints.x] * a_weights.x +
						  		  sbo_skin.transforms[transformIdx + a_joints.y] * a_weights.y +
						  		  sbo_skin.transforms[transformIdx + a_joints.z] * a_weights.z +
						  		  sbo_skin.transforms[transformIdx + a_joints.w] * a_weights.w;

	const mat4 skin = mat4(
		skin3x4[0],
		skin3x4[1],
		skin3x4[2],
		vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	const int resultOffset = int(vertexId + skeleton_ids.data[objectId + push.firstObject + push.skeletonsOffset]);

	imageStore(out_positions, resultOffset, vec4(texelFetch(u_positions, vertexId).xyz, 1.0f) * skin);

	const mat3 skinIT = mat3(skin3x4);
	
	const vec4 normal = texelFetch(u_normals, vertexId);
	imageStore(out_normals, resultOffset, vec4(normalize(normal.xyz * skinIT), 0.0f));

	const vec4 tangent = texelFetch(u_tangents, vertexId);
	imageStore(out_tangents, resultOffset, vec4(normalize(tangent.xyz * skinIT), tangent.w));
}