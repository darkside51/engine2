#version 450

layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

layout(push_constant) uniform Params {
	uint oneChannelTexture;
} push;

void main() {
	vec4 textureColor = texture(texture0, v_uv);
	out_color = v_color * textureColor.rrrr;
	out_color = v_color * (push.oneChannelTexture == 1 ? textureColor.rrrr : textureColor);
}