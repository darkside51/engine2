#version 450

layout(set = 0, binding = 1) uniform sampler2D texture0;

layout (location = 0) in vec2 v_uv;
layout (location = 1) in float v_time;
//layout (location = 1) in vec3 v_color;
layout (location = 0) out vec4 out_color;

//void main() {
	//out_color = texture(texture0, v_uv);// * vec4(v_color, 1.0);
	//out_color.rgb = pow(out_color.rgb, vec3(1.0/2.2));
//}


const float coeiff = 0.25;
const vec3 totalSkyLight = vec3(0.3, 0.5, 1.0);

vec3 mie(float dist, vec3 sunL){
    return max(exp(-pow(dist, 0.25)) * sunL - 0.4, 0.0);
}

vec3 getSky(vec2 uv){
	
	vec2 sunPos = vec2(0.5, cos(v_time * 0.3 + 3.14 * 0.564));
    
    float sunDistance = distance(uv, clamp(sunPos, -1.0, 1.0));
	
	float scatterMult = clamp(sunDistance, 0.0, 1.0);
	float sun = clamp(1.0 - smoothstep(0.01, 0.011, scatterMult), 0.0, 1.0);
	
	float dist = uv.y;
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;
    
    vec3 mieScatter = mie(sunDistance, vec3(1.0));
	
	vec3 color = dist * totalSkyLight;
    
    color = max(color, 0.0);
    
	color = max(mix(pow(color, 1.0 - color),
	color / (2.0 * color + 0.5 - color),
	clamp(sunPos.y * 2.0, 0.0, 1.0)),0.0)
	+ sun + mieScatter;
	
	color *= (pow(1.0 - scatterMult, 10.0) * 10.0) + 1.0;
	
	float underscatter = distance(sunPos.y * 0.5 + 0.5, 1.0);
	
	color = mix(color, vec3(0.0), clamp(underscatter, 0.0, 1.0));
	
	return color;	
}

vec3 getSky2(vec2 uv){
	vec2 sunPos = vec2(0.5, cos(v_time + 3.14));
	//vec2 sunPos = vec2(0.5, 0.15);

    float sunDistance = distance(uv, clamp(sunPos, -1.0, 1.0));
	
	float scatterMult = clamp(sunDistance, 0.0, 1.0);
	float sun = clamp(1.0 - smoothstep(0.01, 0.011, scatterMult), 0.0, 1.0);
	
	float dist = min(uv.y, 0.5 - uv.y);
	dist = (coeiff * mix(scatterMult, 1.0, dist)) / dist;
    
    vec3 mieScatter = mie(sunDistance, vec3(1.0));
	
	vec3 color = dist * totalSkyLight;
    
    color = max(color, 0.0);
    
	float underscatter = 0.5 - distance(sunPos.y, 0.25);

	vec3 darkColor = totalSkyLight * 0.12;
	color = max(
		mix(pow(color, 1.0 - color), color / (2.0 * color + 0.5 - color), clamp(underscatter * 2.0, 0.0, 1.0)),
		darkColor)
	+ sun + mieScatter;
	
	color *= (pow(1.0 - scatterMult, 10.0) * 10.0) + 1.0;
	
	color = mix(color, darkColor, clamp(abs(sunPos.y), 0.0, 1.0));
	
	return color;	
}

void main(){
	//vec3 color = getSky(vec2(v_uv.x, 1.0 - v_uv.y));
	vec3 color = getSky2(vec2(v_uv.x, 0.5 * (1.0 - v_uv.y)));
	color = color / (2.0 * color + 0.5 - color);
	out_color = vec4(color, 1.0) * texture(texture0, v_uv);
}


/*const float R0 = 6360e3; // Earth surface radius
const float Ra = 6380e3; // Earth atmosphere top raduis
const vec3 bR = vec3(58e-7, 135e-7, 331e-7); // Rayleigh scattering coefficient
const vec3 bMs = vec3(2e-5); // Mie scattering coefficients
const vec3 bMe = bMs * 1.1;
const float I = 10.; // Sun intensity
const vec3 C = vec3(0., -R0, 0.); // Earth center point

// Calculate densities $\rho$.
// Returns vec2(rho_rayleigh, rho_mie)
// Note that intro version is more complicated and adds clouds by abusing Mie scattering density. That's why it's a separate function
vec2 densitiesRM(vec3 p) {
	float h = max(0., length(p - C) - R0); // calculate height from Earth surface
	return vec2(exp(-h/8e3), exp(-h/12e2));
}

// Basically a ray-sphere intersection. Find distance to where rays escapes a sphere with given radius.
// Used to calculate length at which ray escapes atmosphere
float escape(vec3 p, vec3 d, float R) {
	vec3 v = p - C;
	float b = dot(v, d);
	float det = b * b - dot(v, v) + R*R;
	if (det < 0.) return -1.;
	det = sqrt(det);
	float t1 = -b - det, t2 = -b + det;
	return (t1 >= 0.) ? t1 : t2;
}

// Calculate density integral for optical depth for ray starting at point `p` in direction `d` for length `L`
// Perform `steps` steps of integration
// Returns vec2(depth_int_rayleigh, depth_int_mie)
vec2 scatterDepthInt(vec3 o, vec3 d, float L, float steps) {
	// Accumulator
	vec2 depthRMs = vec2(0.);

	// Set L to be step distance and pre-multiply d with it
	L /= steps; d *= L;
	
	// Go from point P to A
	for (float i = 0.; i < steps; ++i)
		// Simply accumulate densities
		depthRMs += densitiesRM(o + d * i);

	return depthRMs * L;
}


// Global variables, needed for size
vec2 totalDepthRM;
vec3 I_R, I_M;

vec3 sundir;

// Calculate in-scattering for ray starting at point `o` in direction `d` for length `L`
// Perform `steps` steps of integration
void scatterIn(vec3 o, vec3 d, float L, float steps) {

	// Set L to be step distance and pre-multiply d with it
	L /= steps; d *= L;

	// Go from point O to B
	for (float i = 0.; i < steps; ++i) {

		// Calculate position of point P_i
		vec3 p = o + d * i;

		// Calculate densities
		vec2 dRM = densitiesRM(p) * L;

		// Accumulate T(P_i -> O) with the new P_i
		totalDepthRM += dRM;

		// Calculate sum of optical depths. totalDepthRM is T(P_i -> O)
		// scatterDepthInt calculates integral part for T(A -> P_i)
		// So depthRMSum becomes sum of both optical depths
		vec2 depthRMsum = totalDepthRM + scatterDepthInt(p, sundir, escape(p, sundir, Ra), 4.);

		// Calculate e^(T(A -> P_i) + T(P_i -> O)
		vec3 A = exp(-bR * depthRMsum.x - bMe * depthRMsum.y);

		// Accumulate I_R and I_M
		I_R += A * dRM.x;
		I_M += A * dRM.y;
	}
}

// Final scattering function
// O = o -- starting point
// B = o + d * L -- end point
// Lo -- end point color to calculate extinction for
vec3 scatter(vec3 o, vec3 d, float L, vec3 Lo) {

	// Zero T(P -> O) accumulator
	totalDepthRM = vec2(0.);

	// Zero I_M and I_R
	I_R = I_M = vec3(0.);

	// Compute T(P -> O) and I_M and I_R
	scatterIn(o, d, L, 16.);

	// mu = cos(alpha)
	float mu = dot(d, sundir);

	// Calculate Lo extinction
	return Lo * exp(-bR * totalDepthRM.x - bMe * totalDepthRM.y)

	// Add in-scattering
		+ I * (1. + mu * mu) * (
			I_R * bR * .0597 +
			I_M * bMs * .0196 / pow(1.58 - 1.52 * mu, 1.5));
}


void main() {
	// Normalized pixel coordinates (from -1 to 1)
    vec2 uv = vec2(v_uv.x + 0.25, 1.0 - v_uv.y + 0.25) * 2. - 1.;
    // Fix aspect
    uv.x *= 2.0;
    
    sundir = normalize(vec3(.5, .4 * (1. + sin(.5 * v_time)), -1.));
    
    vec3 O = vec3(0., 0., 0.);
    vec3 D = normalize(vec3(uv, -2.));
    
    vec3 col = vec3(0.);
    float L = escape(O, D, Ra);
    col = scatter(O, D, L, col);

    out_color = vec4(sqrt(col), 1.);
}*/