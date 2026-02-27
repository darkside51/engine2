/*#version 450 - use same

// Use imageBuffer for read/write access
layout(set = 0, binding = 0, rgba32f) uniform imageBuffer uStorageTexelBuffer;

void main() {
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.x, 0);
    
    // Read
    vec4 data = imageLoad(uStorageTexelBuffer, gl_GlobalInvocationID.x);
    
    // Modify
    data += vec4(0.1);
    
    // Write
    imageStore(uStorageTexelBuffer, gl_GlobalInvocationID.x, data);
}*/

/*
#version 450
layout(binding = 0) uniform samplerBuffer myTexelBuffer;
void main() {
    // Get the index of the texel you want to access
    int index = ...; // e.g., from a vertex attribute, a global invocation ID in compute shader, etc.

    // Fetch the data at that specific index
    vec4 value = texelFetch(myTexelBuffer, index);

    // Use the retrieved 'value'
    // ...
}
*/

#version 450

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// out data
layout (set = 0, binding = 0) buffer OutPositions {
   vec4 positions[];
} out_positions;

/*layout (set = 0, binding = 1) buffer OutNormals {
   vec4 normals[];
} out_normals;*/ 

// storage buffers
layout (set = 1, binding = 0) readonly buffer SkinTransformations {
   mat3x4 transforms[];
} sbo_skin;

// in data
layout (set = 2, binding = 0) readonly buffer Positions {
   vec4 positions[];
} sbo_positions;

layout (set = 2, binding = 1) readonly buffer Normals {
   vec4 normals[];
} sbo_normals;

layout (set = 2, binding = 2) readonly buffer Joints {
   uvec4 joints[];
} sbo_joints;

layout (set = 2, binding = 3) readonly buffer Weights {
   vec4 weights[];
} sbo_weights;

layout(push_constant) uniform Data {
	uint objectsCount;
	uint vertexPerObjexts;
	uint transformOffset;
} push;

void main() {
	uint invocationId = gl_GlobalInvocationID.x;
	uint objectId = invocationId / push.vertexPerObjexts;

	if (objectId > push.objectsCount) {
		return;
	}

	objectId += push.transformOffset;
	uint vertexId = invocationId % push.vertexPerObjexts;

	uvec4 a_joints = sbo_joints.joints[vertexId];
	vec4 a_weights = sbo_weights.weights[vertexId];

	const mat4 skin = mat4(
		sbo_skin.transforms[objectId + a_joints.x][0],
		sbo_skin.transforms[objectId + a_joints.x][1],
		sbo_skin.transforms[objectId + a_joints.x][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.x +
	mat4(
		sbo_skin.transforms[objectId + a_joints.y][0],
		sbo_skin.transforms[objectId + a_joints.y][1],
		sbo_skin.transforms[objectId + a_joints.y][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.y +
	mat4(
		sbo_skin.transforms[objectId + a_joints.z][0],
		sbo_skin.transforms[objectId + a_joints.z][1],
		sbo_skin.transforms[objectId + a_joints.z][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.z +
	mat4(
		sbo_skin.transforms[objectId + a_joints.w][0],
		sbo_skin.transforms[objectId + a_joints.w][1],
		sbo_skin.transforms[objectId + a_joints.w][2],
		vec4(0.0, 0.0, 0.0, 1.0)
	) * a_weights.w;

	out_positions.positions[invocationId] = vec4(sbo_positions.positions[vertexId].xyz, 1.0) * skin;
	//out_normals.normals[invocationId] = normalize((sbo_normals.normals[vertexId] * skin));
}