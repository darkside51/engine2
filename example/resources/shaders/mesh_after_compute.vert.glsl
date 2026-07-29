#version 450

layout (location = 0) in vec2 a_uv;
//layout (location = 1) in uint a_skeleton_ids; // per instance value

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
	float time;
} u_viewParams;

//layout (set = 1, binding = 0) readonly buffer Positions {
//   vec4 data[];
//} in_positions;

//layout (set = 1, binding = 1) readonly buffer Normals {
//   vec4 data[];
//} in_normals;

layout(set = 1, binding = 0, rgba32f) readonly uniform imageBuffer in_positions;
layout(set = 1, binding = 1, rgba8_snorm) readonly uniform imageBuffer in_normals;
layout(set = 1, binding = 3, rgba8_snorm) readonly uniform imageBuffer in_tangents;

layout (set = 1, binding = 2) readonly buffer SkeletonIds {
   uint data[];
} in_skeleton_ids;

//layout (set = 1, binding = 3) readonly buffer Tangents {
//   vec4 data[];
//} in_tangents;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out mat3 v_tbn;

layout(push_constant) uniform Params {
	vec4 color;
	vec4 emissive;
	vec4 metalicRoughness;
	uvec4 offset;
} push;

void main() {
	const uint vertexId = gl_VertexIndex + in_skeleton_ids.data[gl_InstanceIndex + push.offset.x];//a_skeleton_ids;
	const vec4 position = imageLoad(in_positions, int(vertexId)); // in_positions.data[vertexId];
	gl_Position = u_viewParams.viewProjection * position;
	v_uv = a_uv;

	//const mat3 normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
	
	//const vec3 normal = imageLoad(in_normals, int(vertexId)).xyz;//in_normals.data[vertexId].xyz;
	//const vec4 tangent = imageLoad(in_tangents, int(vertexId));//in_tangents.data[vertexId];
	//const vec3 binormal = cross(normal, tangent.xyz) * tangent.w;

	//v_tbn = mat3(normalMatrix * tangent.xyz, normalMatrix * binormal, normalMatrix * normal);

   const mat3 normalMatrix = transpose(inverse(mat3(u_viewParams.view)));
   //const mat3 normalMatrix = mat3(u_viewParams.view);
    
   const vec3 normal = imageLoad(in_normals, int(vertexId)).xyz;
   const vec4 tangent = imageLoad(in_tangents, int(vertexId));
    
   vec3 T = normalize(normalMatrix * tangent.xyz);
   const vec3 N = normalize(normalMatrix * normal);
    
   // Грама-Шмидта: гарантирует строгую ортогональность базиса
   T = normalize(T - N * dot(N, T));
    
   const vec3 B = normalize(cross(N, T) * tangent.w);
   v_tbn = mat3(T, B, N);
}