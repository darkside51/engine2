#version 450

layout (local_size_x = 32, local_size_y = 32, local_size_z = 1) in;


layout(set = 0, binding = 0, rgba8_snorm) writeonly uniform image2D normals;
layout(set = 0, binding = 1) uniform sampler2D heights;

const float cellSize = 1.0f;

void main() {
	const ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
   const ivec2 texSize = textureSize(heights, 0);

   if (texCoord.x >= texSize.x || texCoord.y >= texSize.y) {
      return;
   }

   const vec2 texelSize = 1.0f / vec2(texSize);
   const vec2 uv = (vec2(texCoord) + 0.5f) * texelSize;

   const float hLeft  = textureOffset(heights, uv, ivec2(-1,  0)).r;
   const float hRight = textureOffset(heights, uv, ivec2( 1,  0)).r;
   const float hDown  = textureOffset(heights, uv, ivec2( 0, -1)).r;
   const float hUp    = textureOffset(heights, uv, ivec2( 0,  1)).r;

   const vec3 tangent   = vec3(2.0f * cellSize, 0.0f, hRight - hLeft);
   const vec3 bitangent = vec3(0.0f, 2.0f * cellSize, hUp - hDown);

   const vec3 normal = normalize(cross(bitangent, tangent)); // (+z up)
   imageStore(normals, texCoord, vec4(normal, 0.0f));
}