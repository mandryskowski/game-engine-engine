#define M_PI 3.14159
#define SAMPLE_COUNT 4096u
#define SAMPLE_CUBEMAP_RESOLUTION 1024.0

//in
in vec3 localPosition;

//out
out vec4 fragColor;

//uniform
uniform float roughness;
uniform float cubemapNr;
uniform samplerCube cubemap;

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * M_PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	//Spherical -> Cartesian
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	vec3 up = (abs(N.z) > 0.999) ? (vec3(1.0, 0.0, 0.0)) : (vec3(0.0, 0.0, 1.0));
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	return tangent * H.x + bitangent * H.y + N * H.z;
}

float DistributionGGX(float NdotH, float alpha)
{
	return (alpha * alpha) / (M_PI * pow((pow(NdotH, 2.0) * (pow(alpha, 2.0) - 1.0) + 1.0), 2.0));
}

void main()
{
	vec3 N = normalize(localPosition);
	vec3 R = N;
	vec3 V = R;
	
	vec3 sum = vec3(0.0);
	float weightSum = 0.0;
	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H = ImportanceSampleGGX(Xi, N, roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);
		float NdotL = max(dot(N, L), 0.0);
		float NdotH = max(dot(N, H), 0.001);
		float HdotV = max(dot(H, V), 0.001);
		
		
		float D = DistributionGGX(NdotH, roughness * roughness);
		float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
		
		float saTexel = 4.0 * M_PI / (6.0 * SAMPLE_CUBEMAP_RESOLUTION * SAMPLE_CUBEMAP_RESOLUTION);
		float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
		
		float mipLevel = (roughness == 0.0) ? (0.0) : (0.5 * log2(saSample / saTexel));
		
		if (NdotL > 0.0)
		{
			sum += textureLod(cubemap, vec3(L), 0).rgb * NdotL;
			weightSum += NdotL;
		}
	}
	
	fragColor = vec4(sum / weightSum, 1.0);
}