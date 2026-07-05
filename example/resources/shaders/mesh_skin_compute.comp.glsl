#version 450

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// out data
//layout (set = 0, binding = 0) writeonly buffer OutPositions {
//   vec4 data[];
//} out_positions;

//layout (set = 0, binding = 1) writeonly buffer OutNormals {
//   vec4 data[];
//} out_normals;

//layout (set = 0, binding = 2) writeonly buffer OutTangents {
//   vec4 data[];
//} out_tangents;


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

	const uint transfromIdx = skin_offsets.data[objectId + push.firstObject + push.transformsOffset];
	const int vertexId = int(u_indices.data[invocationId % push.invocationsPerObject + push.indicesOffset]);

	const vec4 a_weights = texelFetch(u_weights, vertexId);
	const uvec4 a_joints = uvec4(texelFetch(u_joints, vertexId) * 255.0);

	const mat3x4 transforms[] = mat3x4[](
		sbo_skin.transforms[transfromIdx + a_joints.x],
		sbo_skin.transforms[transfromIdx + a_joints.y],
		sbo_skin.transforms[transfromIdx + a_joints.z],
		sbo_skin.transforms[transfromIdx + a_joints.w]
	);

	const mat4 skin = mat4(
		transforms[0][0],
		transforms[0][1],
		transforms[0][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.x +
	mat4(
		transforms[1][0],
		transforms[1][1],
		transforms[1][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.y +
	mat4(
		transforms[2][0],
		transforms[2][1],
		transforms[2][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.z +
	mat4(
		transforms[3][0],
		transforms[3][1],
		transforms[3][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.w;

	//const uint skeletonData = skeleton_ids.data[objectId + push.firstObject];
	//const uint skeletonId = skeletonData & 0xffffu;
	//const uint vertexCount = (skeletonData >> 16u) & 0xffffu;
	//const uint resultOffset = vertexId + skeletonId * vertexCount;

	const uint resultOffset = vertexId + skeleton_ids.data[objectId + push.firstObject + push.skeletonsOffset];
	//out_positions.data[resultOffset] = vec4(texelFetch(u_positions, vertexId).xyz, 1.0) * skin;
	//out_normals.data[resultOffset] = normalize((texelFetch(u_normals, vertexId) * skin));
	//out_tangents.data[resultOffset] = normalize((texelFetch(u_tangents, vertexId) * skin));

	imageStore(out_positions, int(resultOffset), vec4(texelFetch(u_positions, vertexId).xyz, 1.0) * skin);
	imageStore(out_normals, int(resultOffset), normalize((texelFetch(u_normals, vertexId) * skin)));
	imageStore(out_tangents, int(resultOffset), normalize((texelFetch(u_tangents, vertexId) * skin)));
}