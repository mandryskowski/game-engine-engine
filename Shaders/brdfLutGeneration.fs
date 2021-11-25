#define M_PI 3.14159
#define SAMPLE_COUNT 1024u
//in
in vec2 texCoord;

//out
out vec2 fragColor;

//uniform
uniform sampler2D tex;

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

float GeometrySchlickGGX(float NdotV, float k)
{
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotV, float NdotL, float k)
{
	return GeometrySchlickGGX(NdotV, k) * GeometrySchlickGGX(NdotL, k);
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
	vec3 V;
	V.x = sqrt(1.0 - NdotV*NdotV);
	V.y = 0.0;
	V.z = NdotV;
	
	float scale = 0.0;
	float bias = 0.0;
	
	float k = (roughness * roughness) / 2.0;
	vec3 N = vec3(0.0, 0.0, 1.0);

	for (uint i = 0u; i < SAMPLE_COUNT; i++)
	{
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		vec3 H = ImportanceSampleGGX(Xi, N, roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);
		
		float NdotL = max(L.z, 0.0);
		float NdotV = max(V.z, 0.0);
		float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(H, V), 0.0);
		
		if (NdotL > 0.0)
		{
			float G = GeometrySmith(NdotV, NdotL, k);
			float G_Vis = (G * VdotH) / (NdotH * NdotV);
			float Fc = pow(1.0 - VdotH, 5.0);
			
			scale += (1.0 - Fc) * G_Vis;
			bias += Fc * G_Vis;
		}
	}
	
	return vec2(scale, bias) / vec2(float(SAMPLE_COUNT));
}

void main()
{
	fragColor = IntegrateBRDF(texCoord.x, texCoord.y);
}