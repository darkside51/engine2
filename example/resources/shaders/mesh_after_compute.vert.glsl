#version 450

layout (location = 0) in vec2 a_uv;
//layout (location = 1) in uint a_skeleton_ids; // per instance value

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	mat3 viewTransposeInverse;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;


layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;
layout(set = 1, binding = 1, rgba8_snorm) readonly uniform imageBuffer in_normals;
layout(set = 1, binding = 2, rgba8_snorm) readonly uniform imageBuffer in_tangents;

layout (set = 1, binding = 3) readonly buffer SkeletonIds {
   uint data[];
} in_skeleton_ids;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out mat3 v_tbn;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	const uint id = in_skeleton_ids.data[gl_InstanceIndex + push.offset.x];
	const int vertexId = int(gl_VertexIndex + id);//a_skeleton_ids;
	const vec4 position = imageLoad(in_positions, vertexId); // in_positions.data[vertexId];
	gl_Position = u_viewParams.viewProjection * position;
	v_uv = a_uv;
    
   const vec3 normal = imageLoad(in_normals, vertexId).xyz;
   const vec4 tangent = imageLoad(in_tangents, vertexId);
    
   const vec3 t = normalize(u_viewParams.viewTransposeInverse * tangent.xyz);
   const vec3 n = normalize(u_viewParams.viewTransposeInverse * normal); 
   const vec3 b = normalize(cross(n, t) * -tangent.w);

   v_tbn = mat3(t, b, n);
}