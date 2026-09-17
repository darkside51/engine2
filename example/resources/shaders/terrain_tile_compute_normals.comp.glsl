#version 450

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout (set = 0, binding = 0) buffer positions {
   vec4 xyzw[];
} in_positions;

layout(set = 0, binding = 1, rgba8_snorm) writeonly uniform image2D normals;

layout(push_constant, std430) uniform Transform {
	uint gridSize;
} push;

const float cellSize = 1.0f;

float getHeight(uint x, uint y) {
    const uint clampedX = clamp(x, 0u, push.gridSize - 1u);
    const uint clampedY = clamp(y, 0u, push.gridSize - 1u);
    return in_positions.xyzw[clampedY * push.gridSize + clampedX].z;
}

void main() {
	uint x = gl_GlobalInvocationID.x;
   uint y = gl_GlobalInvocationID.y;

   if (x >= push.gridSize || y >= push.gridSize) {
   	return;
   }

   const float hLeft  = getHeight(x - 1u, y);
   const float hRight = getHeight(x + 1u, y);
   const float hDown  = getHeight(x, y - 1u);
   const float hUp    = getHeight(x, y + 1u);

   const vec3 tangent   = vec3(2.0f * cellSize, 0.0f, hRight - hLeft);
   const vec3 bitangent = vec3(0.0f, 2.0f * cellSize, hUp - hDown);

	const ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
   const vec3 normal = normalize(cross(bitangent, tangent)); // (+z up)
   imageStore(normals, texCoord, vec4(normal, 1.0));
}