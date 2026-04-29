#version 450

layout (constant_id = 0) const int kSsaoKernelSize = 32;
layout (constant_id = 1) const float kSsaoRadius = 0.08;
layout (constant_id = 2) const float bias = 0.010f;

layout(set = 0, binding = 0) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D ssaoNoise;

layout (set = 0, binding = 3) uniform UBOSSAOKernel {
	vec4 samples[kSsaoKernelSize];
} uboSSAOKernel;

layout(push_constant) uniform Params {
	mat4 projection;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out float out_color;

void main() {
	// gBuffer values
	vec3 fragPos = texture(texturePositionDepth, v_uv).rgb;
	vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;

	// get a random vector using a noise lookup
	ivec2 texDim = textureSize(texturePositionDepth, 0); 
	ivec2 noiseDim = textureSize(ssaoNoise, 0);
	const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * v_uv;  
	vec3 randomVec = vec3(texture(ssaoNoise, noiseUV).xy * 2.0 - 1.0, -1.0);

	// create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
	for(int i = 0; i < kSsaoKernelSize; i++) {		
		vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz; 
		samplePos = fragPos + samplePos * kSsaoRadius; 
		
		// project
		vec4 offset = push.projection * vec4(samplePos, 1.0f);
		offset.xyz /= offset.w; 
		offset.xyz = offset.xyz * 0.5f + 0.5f; 
		
		const float sampleDepth = -texture(texturePositionDepth, offset.xy).w; 
		const float rangeCheck = smoothstep(0.0f, 1.0f, kSsaoRadius / abs(fragPos.z - sampleDepth));

		occlusion += (sampleDepth >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;           
	}
	occlusion = 1.0 - (occlusion / float(kSsaoKernelSize));
	out_color = occlusion;
}