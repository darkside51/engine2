#version 450

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// out data
layout (set = 0, binding = 0) buffer OutPositions {
   vec4 data[];
} out_positions;

layout (set = 0, binding = 1) buffer OutNormals {
   vec4 data[];
} out_normals;

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

layout(push_constant) uniform Data {
	uint firstObject;
	uint objectsCount;
	uint invocationsPerObject;
	uint indicesOffset;
	uint elementsCount;
	uint resultOffset;
} push;

void main() {
	const uint invocationId = gl_GlobalInvocationID.x;

	//const uint invocationId = gl_GlobalInvocationID.x + (gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x) + (gl_GlobalInvocationID.z * 
	//	gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y);

	const uint objectId = invocationId / push.invocationsPerObject;

	if (objectId >= push.objectsCount) {
		return;
	}

	const uint transfromIdx = skin_offsets.data[objectId + push.firstObject];
	const int vertexId = int(u_indices.data[invocationId % push.invocationsPerObject + push.indicesOffset]);

	const vec4 a_weights = texelFetch(u_weights, vertexId);
	const uvec4 a_joints = uvec4(texelFetch(u_joints, vertexId) * 255.0);

	const mat4 skin = mat4(
		sbo_skin.transforms[transfromIdx + a_joints.x][0],
		sbo_skin.transforms[transfromIdx + a_joints.x][1],
		sbo_skin.transforms[transfromIdx + a_joints.x][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.x +
	mat4(
		sbo_skin.transforms[transfromIdx + a_joints.y][0],
		sbo_skin.transforms[transfromIdx + a_joints.y][1],
		sbo_skin.transforms[transfromIdx + a_joints.y][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.y +
	mat4(
		sbo_skin.transforms[transfromIdx + a_joints.z][0],
		sbo_skin.transforms[transfromIdx + a_joints.z][1],
		sbo_skin.transforms[transfromIdx + a_joints.z][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.z +
	mat4(
		sbo_skin.transforms[transfromIdx + a_joints.w][0],
		sbo_skin.transforms[transfromIdx + a_joints.w][1],
		sbo_skin.transforms[transfromIdx + a_joints.w][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.w;

	//const uint resultOffset = vertexId + objectId * push.elementsCount + push.resultOffset;
	const uint resultOffset = vertexId + skeleton_ids.data[objectId + push.firstObject] * 10000;
	out_positions.data[resultOffset] = vec4(texelFetch(u_positions, vertexId).xyz, 1.0) * skin;
	out_normals.data[resultOffset] = normalize((texelFetch(u_normals, vertexId) * skin));
}