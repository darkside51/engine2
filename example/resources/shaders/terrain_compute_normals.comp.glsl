#version 450

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform utextureBuffer tileIds;
layout(set = 0, binding = 1) uniform sampler2D heights;
layout(set = 0, binding = 2, rgba8_snorm) writeonly uniform image2D normals;

layout(push_constant, std430) uniform Params {
   uint idsBufferOffset;
   uint tilesInRaw;
   uint groupWidth;
} push;

const uint kVerticesInRow = 32u;
const float cellSize = 1.0f;

void main() {
   const uint group_linear_id = gl_WorkGroupID.y * push.groupWidth + gl_WorkGroupID.x;
   const uint tileID = texelFetch(tileIds, int(push.idsBufferOffset + group_linear_id)).x;

   const int y = int(tileID / push.tilesInRaw * kVerticesInRow);
   const int x = int((tileID & (push.tilesInRaw - 1u)) * kVerticesInRow);

	const ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy & (kVerticesInRow - 1u)) + ivec2(x, y);
   const ivec2 texSize = textureSize(heights, 0);

   if (texCoord.x >= texSize.x || texCoord.y >= texSize.y) {
      return;
   }

   const vec2 texelSize = 1.0f / vec2(texSize);
   const vec2 uv = vec2(texCoord) * texelSize;

   const float hCenter = texture(heights, (vec2(texCoord) + 0.5f) * texelSize).r;
   const float hLeft  = texCoord.x > 1 ? textureOffset(heights, uv, ivec2(-1,  0)).r : hCenter;
   const float hRight = texCoord.x < texSize.x - 1 ? textureOffset(heights, uv, ivec2( 1,  0)).r : hCenter;
   const float hDown  = texCoord.y > 1 ? textureOffset(heights, uv, ivec2( 0, -1)).r : hCenter;
   const float hUp    = texCoord.y < texSize.y - 1 ? textureOffset(heights, uv, ivec2( 0,  1)).r : hCenter;
   
   const vec3 tangent   = vec3(2.0f * cellSize, 0.0f, hRight - hLeft);
   const vec3 bitangent = vec3(0.0f, 2.0f * cellSize, hUp - hDown);

   const vec3 normal = normalize(cross(bitangent, tangent)); // (+z up)
   imageStore(normals, texCoord, vec4(normal, 0.0f));
}