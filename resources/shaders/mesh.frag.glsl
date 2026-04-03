#version 450

layout (location = 0) in vec3 v_normal;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec4 v_color;
layout (location = 3) in vec3 v_emissive;

layout(set = 1, binding = 0) uniform sampler2D u_texture;
layout(set = 1, binding = 1) uniform sampler2D u_texture_emissive;

layout (location = 0) out vec4 out_color;

const vec3 lightDirection = normalize(vec3(0.45, 0.4, -0.55));
const vec3 upDirection = normalize(vec3(0.0, 0.0, 1.0));

void main() {
	const float ambient = 0.2;
	const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
	const float light = ambient + max(dot(v_normal, -lightDirection), 0.0);
	//const float light2 = 0.25 * clamp(max(dot(v_normal, -reflect(lightDirection, upDirection)), 0.0), 0.0, 1.0);
	out_color = (texture(u_texture, v_uv) * v_color) * lightColor * light;// * vec4(1.0, 0.0, 0.0, 1.0);
	out_color.rgb += texture(u_texture_emissive, v_uv).rgb * v_emissive;
}