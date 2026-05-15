#version 450

#import common

layout (set = 0, binding = 0) uniform UniformBuffer {
	mat4 view;
	mat4 projection;
	mat4 viewProjection;
	vec3 cameraPosition;
	vec2 nearFarPlanes;
} u_viewParams;

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec3 v_emissive;
//layout (location = 4) in vec3 v_viewPosition;
layout (location = 4) in vec3 v_worldPosition;

layout(set = 2, binding = 0) uniform sampler2D u_texture;
layout(set = 2, binding = 1) uniform sampler2D u_texture_emissive;

//layout (location = 0) out vec4 out_color;

layout (location = 0) out vec4 out_position;
//layout (location = 1) out vec4 out_world_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_albedo;
layout (location = 3) out vec4 out_emissive;

//const vec3 lightDirection = normalize(vec3(0.45, 0.4, -0.55));
//const vec3 upDirection = normalize(vec3(0.0, 0.0, 1.0));

void main() {
	//const float ambient = 0.1;
	//const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
	//const float light = ambient + max(dot(v_normal, -lightDirection), 0.0);
	//const float light2 = 0.25 * clamp(max(dot(v_normal, -reflect(lightDirection, upDirection)), 0.0), 0.0, 1.0);
	//out_color = (texture(u_texture, v_uv) * v_color) * lightColor * light;// * vec4(1.0, 0.0, 0.0, 1.0);
	//out_color.rgb += texture(u_texture_emissive, v_uv).rgb * v_emissive;

	const float depthLinear = linearDepth(gl_FragCoord.z, u_viewParams.nearFarPlanes.x, u_viewParams.nearFarPlanes.y);
	//out_position = vec4(v_viewPosition, depthLinear);
	out_position = vec4(v_worldPosition, depthLinear);
	
	out_normal = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);
	out_albedo = texture(u_texture, v_uv) * v_color;
	out_emissive = vec4(texture(u_texture_emissive, v_uv).rgb * v_emissive, 1.0);
	//out_world_position = vec4(v_worldPosition, depthLinear);
}