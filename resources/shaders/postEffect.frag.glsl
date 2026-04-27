#version 450

/*
layout(set = 0, binding = 0) uniform sampler2D texture0;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

void main() {
	out_color = texture(texture0, v_uv);
	out_color.rgb = pow(out_color.rgb, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
}
*/

layout(set = 0, binding = 0) uniform sampler2D texturePositionDepth;
layout(set = 0, binding = 1) uniform sampler2D textureNormal;
layout(set = 0, binding = 2) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 3) uniform sampler2D ssaoNoise;

layout (constant_id = 0) const int kSsaoKernelSize = 32;
//layout (constant_id = 1) const float kSsaoRadius = 0.5;
//const float bias = 0.025f;

layout (constant_id = 1) const float kSsaoRadius = 0.3;
const float bias = 0.02f;

layout (set = 0, binding = 4) uniform UBOSSAOKernel {
	vec4 samples[kSsaoKernelSize];
} uboSSAOKernel;

layout(push_constant) uniform Params {
	mat4 projection;
} push;

layout (location = 0) in vec2 v_uv;
layout (location = 0) out vec4 out_color;

void main() {
	// gBuffer values
	vec3 fragPos = texture(texturePositionDepth, v_uv).rgb;
	vec3 normal = normalize(texture(textureNormal, v_uv).rgb * 2.0 - 1.0);

	// get a random vector using a noise lookup
	ivec2 texDim = textureSize(texturePositionDepth, 0); 
	ivec2 noiseDim = textureSize(ssaoNoise, 0);
	const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * v_uv;  
	vec3 randomVec = texture(ssaoNoise, noiseUV).xyz * 2.0 - 1.0;

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


	vec4 albedo = texture(textureAlbedo, v_uv);
	vec3 lightPos = vec3(0.0);
	vec3 L = normalize(lightPos - fragPos);
	float NdotL = max(0.1, dot(normal, L));
	vec3 baseColor = albedo.rgb;// * NdotL;

	out_color = vec4(baseColor * occlusion, 1.0);

	out_color.rgb = pow(out_color.rgb, vec3(0.4545)); // gamma correction 1.0 / 2.2 = 0.4545
}