#version 450

#import common

layout (constant_id = 0) const int kSsaoKernelSize = 32;
//layout (constant_id = 1) const float kSsaoRadius = 0.2f;
//layout (constant_id = 2) const float bias = 0.007f;
//layout (constant_id = 3) const float kMaxDepth = 16.0f;

layout(set = 0, binding = 0) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D ssaoNoise;

layout (set = 0, binding = 3) uniform UBOSSAOKernel {
	vec4 samples[kSsaoKernelSize];
} uboSSAOKernel;

layout(push_constant) uniform Params {
	mat4 projection;
	//mat4 view; // and try to use world positions texture
	vec4 options;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in vec3 v_rayDir;
layout (location = 0) out float out_color;

void main() {
	const float radius = push.options.x;
	const float bias = push.options.y;
	const float maxDepth = push.options.z;

	//const float radius = 0.11f;
	//const float bias = 0.004f;
	//const float maxDepth = 5.0f;

	// gBuffer values
	//const vec3 fragPos = texture(texturePositionDepth, v_uv).rgb;
	//const vec4 position = texture(texturePositionDepth, v_uv);

	//const vec3 fragPos = (push.view * vec4(position.rgb, 1.0f - step(position.w, 0.0))).xyz;

	const float linearDepth = texture(texturePositionDepth, v_uv).r;
	const vec3 fragPos = reconstructPosition(v_rayDir, vec3(0.0f), linearDepth);

	const vec3 normal = texture(textureNormal, v_uv).rgb * 2.0 - 1.0;

	// get a random vector using a noise lookup
	const ivec2 texDim = textureSize(texturePositionDepth, 0); 
	const ivec2 noiseDim = textureSize(ssaoNoise, 0);
	const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * v_uv;  
	const vec3 randomVec = vec3(texture(ssaoNoise, noiseUV).xy * 2.0 - 1.0, -1.0);

	// create TBN matrix
	const vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	const vec3 bitangent = cross(tangent, normal);
	const mat3 TBN = mat3(tangent, bitangent, normal);

	// calculate occlusion value
	float occlusion = 0.0f;
	// remove banding
	for(int i = 0; i < kSsaoKernelSize; i++) {		
		vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz; 
		//samplePos = fragPos + samplePos * kSsaoRadius;
		samplePos = fragPos + samplePos * radius;
		
		// project
		vec4 offset = push.projection * vec4(samplePos, 1.0f);
		offset.xyz /= offset.w; 
		offset.xyz = offset.xyz * 0.5f + 0.5f; 
		
		const float sampleDepth = -texture(texturePositionDepth, offset.xy).r; 
		//const float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth));
		const float rangeCheck = push.options.w < 0.5f ?
								 smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth)) :
								 clamp(radius / abs(fragPos.z - sampleDepth), 0.0f, 1.0f);

		occlusion += step(samplePos.z + bias, sampleDepth) * (1.0f - step(0.0f, sampleDepth)) * rangeCheck * smoothstep(0.0f, 1.0f, -maxDepth / fragPos.z);       
	}
	
	out_color = 1.0f - (occlusion / float(kSsaoKernelSize));
}