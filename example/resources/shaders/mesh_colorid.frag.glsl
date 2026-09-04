#version 450

layout (location = 0) flat in uint v_colorId;
layout (location = 0) out vec4 out_colorId;

void main() {
	out_colorId = vec4(
		float((v_colorId >> 0) & 0xff) / 255.0f,
		float((v_colorId >> 8) & 0xff) / 255.0f,
		float((v_colorId >> 16) & 0xff) / 255.0f,
		float((v_colorId >> 24) & 0xff) / 255.0f
	);
}