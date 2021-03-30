#define MAX_LIGHTS 16
#define M_PI 3.14159
#define MAX_PREFILTER_MIPMAP 4.0

#if !defined(POINT_LIGHT) && !defined(DIRECTIONAL_LIGHT) && !defined(SPOT_LIGHT) && !defined(IBL_PASS)
#error Cannot compile Cook-Torrance shader without defining POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT or IBL_PASS.
#endif

struct Light
{
	vec4 position;
	vec4 direction;
	
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	
	float attenuation;	//quadratic component of light's attenuation equation
	float cutOff;		//cosine of the angle of the inner falloff cone (spot lights)
	float outerCutOff;	//cos of the outer angle
	float type;			//determines the type of the light - directional, point or spot
	
	float shadowMapNr;
	float far;				//used for lights that use Ominidirectional Shadow Mapping (point lights)
	mat4 lightSpaceMatrix;	//used for lights that use 2D shadowmaps (directional and spot lights)
};

struct Fragment
{
	vec3 position;
	vec3 normal;
	vec3 albedo;
	vec3 alphaMetalAo;
	float specular;
	float kDirect;
	vec3 v;
	vec3 n;
	vec3 F0;
	#if defined(ENABLE_SSAO)
	float ambient;
	#endif
};

//out
layout (location = 0) out vec4 fragColor;
#ifdef ENABLE_BLOOM
layout (location = 1) out vec4 brightColor;
#endif

//uniform
uniform int lightIndex;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D gAlphaMetalAo;

uniform samplerCubeArray shadowCubemaps;
uniform sampler2DArray shadowMaps;
uniform float lightProbeNr;
uniform vec3 lightProbePositions[5];
uniform samplerCubeArray irradianceCubemaps;
uniform samplerCubeArray prefilterCubemaps;
uniform sampler2D BRDFLutTex;

#ifdef ENABLE_SSAO
uniform sampler2D ssaoTex;
#endif

layout (std140) uniform Lights
{
	int lightCount;
	vec4 camPos;
	Light lights[MAX_LIGHTS];
};


////////////////////////////////
////////////////////////////////
////////////////////////////////

#ifdef POINT_LIGHT
float CalcShadow3D(Light light, vec3 fragPosition)
{
	vec3 lightToFrag = fragPosition - light.position.xyz;

	return ((length(lightToFrag) / light.far > texture(shadowCubemaps, vec4(lightToFrag, light.shadowMapNr)).r) ? (1.0) : (0.0));
}
#else
float CalcShadow2D(Light light, vec3 fragPosition)
{
	vec4 lightProj = light.lightSpaceMatrix * vec4(fragPosition, 1.0);
	vec3 lightCoords = lightProj.xyz /= lightProj.w;
	lightCoords = lightCoords * 0.5 + 0.5;
	
	return (lightCoords.z > texture(shadowMaps, vec3(lightCoords.xy, light.shadowMapNr)).r) ? (1.0) : (0.0);
}
#endif

float DistributionGGX(float NdotH, float alpha)
{
	return (alpha * alpha) / max((M_PI * pow((pow(NdotH, 2.0) * (pow(alpha, 2.0) - 1.0) + 1.0), 2.0)), 0.001);
}

float GeometrySchlickGGX(float NdotV, float k)
{
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float NdotL, float NdotV, float k)
{
	return GeometrySchlickGGX(NdotL, k) * GeometrySchlickGGX(NdotV, k);
}

vec3 FresnelSchlick(float HdotV, vec3 F0)
{
	return F0 + (1.0 - F0) * pow((1.0 - HdotV), 5.0);
}

vec3 FresnelSchlickRoughness(float HdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow((1.0 - HdotV), 5.0);
}

#ifdef IBL_PASS
vec3 GetIrradiance(vec3 pos, vec3 n)
{
	return texture(irradianceCubemaps, vec4(n, lightProbeNr)).rgb;
	vec3 irradiance = vec3(0.0);
	float minLength = 9999.0;
	for (int i = 1; i < 5; i++)
	{
		float len = length(lightProbePositions[i] - pos);
		irradiance += texture(irradianceCubemaps, vec4(n, i)).rgb / len;
		minLength = min(minLength, len);
	}
	return irradiance * minLength;
}
vec3 GetPrefilterColor(vec3 pos, vec3 r, float roughness)
{
	return textureLod(prefilterCubemaps, vec4(r, lightProbeNr), roughness * MAX_PREFILTER_MIPMAP).rgb;
	vec3 prefilterColor = vec3(0.0);
	float minLength = 9999.0;
	for (int i = 1; i < 5; i++)
	{
		float len = length(lightProbePositions[i] - pos);
		prefilterColor += textureLod(prefilterCubemaps, vec4(r, i), roughness * MAX_PREFILTER_MIPMAP).rgb / len;
		minLength = min(minLength, len);
	}   
	return prefilterColor * minLength;	
}
vec3 CalcAmbient(Fragment frag)
{
	if (frag.normal == vec3(0.0))
		return vec3(0.0);
	vec3 n = normalize(frag.normal);
	vec3 v = normalize(camPos.xyz - frag.position);
	float roughness = pow(frag.alphaMetalAo.r, 1.0 / 2.2);
	
	float NdotV = max(dot(n, v), 0.001);
	vec3 F = FresnelSchlickRoughness(NdotV, frag.F0, roughness);
	vec2 brdfLutData = texture(BRDFLutTex, vec2(NdotV, roughness)).rg;
	
	vec3 diffuseAmbient = GetIrradiance(frag.position, n) * frag.albedo;
	vec3 specularAmbient = GetPrefilterColor(frag.position, reflect(-v, n), roughness) * (F * brdfLutData.x + brdfLutData.y);
	
	vec3 kDiffuse = mix((1.0 - F), vec3(0.0), frag.alphaMetalAo.g);
	
	#ifdef ENABLE_SSAO
	return (kDiffuse * diffuseAmbient + specularAmbient) * frag.ambient;
	#else
	return (kDiffuse * diffuseAmbient + specularAmbient);
	#endif
}
#else
vec3 CalcLight(Light light, Fragment frag)
{
	#ifdef DIRECTIONAL_LIGHT
	vec3 l = normalize(-light.direction.xyz);
	#else 
	vec3 l = normalize(light.position.xyz - frag.position);
	#endif
	vec3 v = normalize(camPos.xyz - frag.position);
	vec3 n = normalize(frag.normal);
	vec3 h = normalize(v + l);
	
	float HdotV = max(dot(h, v), 0.001);
	float NdotH = max(dot(n, h), 0.001);
	float NdotL = max(dot(n, l), 0.001);
	float NdotV = max(dot(n, v), 0.001);
	
	float D = DistributionGGX(NdotH, frag.alphaMetalAo.r);
	vec3 F = FresnelSchlick(HdotV, frag.F0);
	float G = GeometrySmith(NdotL, NdotV, frag.kDirect);
	
	vec3 kDiffuse = mix((1.0 - F), vec3(0.0), frag.alphaMetalAo.g);
	
	#ifdef DIRECTIONAL_LIGHT
	float visibility = 1.0 - CalcShadow2D(light, frag.position);
	
	vec3 radiance = light.diffuse.rgb * visibility;
	vec3 ambient = light.ambient.rgb * frag.albedo * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * (1 - Metalness)
	#elif defined(SPOT_LIGHT)
	float visibility = 1.0 -  CalcShadow2D(light, frag.position);
	
	float dist = length(light.position.xyz - frag.position);
	float attenuation = 1.0 / (dist * dist * light.attenuation);
	
	float LdotLDir = max(dot(l, -light.direction.xyz), 0.0);
	float spotIntensity = (LdotLDir - light.outerCutOff) / max((light.cutOff - light.outerCutOff), 0.001);
	
	vec3 radiance = light.diffuse.rgb * attenuation * spotIntensity * visibility;
	vec3 ambient = light.ambient.rgb * frag.albedo * attenuation * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * Attenuation * (1 - Metalness)
	#else
	float visibility = 1.0 - CalcShadow3D(light, frag.position);
	
	float dist = length(light.position.xyz - frag.position);
	float attenuation = 1.0 / (dist * dist * light.attenuation);
	
	vec3 radiance = light.diffuse.rgb * attenuation * visibility;
	vec3 ambient = light.ambient.rgb * frag.albedo * attenuation * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * Attenuation * (1 - Metalness)
	#endif
	
	#ifdef ENABLE_SSAO
	ambient *= frag.ambient;
	#endif

	return ((kDiffuse * frag.albedo / M_PI) + ((D * F * G) / (4.0 * NdotL * NdotV))) * radiance * NdotL + ambient;
}
#endif

void main() 
{
	vec2 texCoord = gl_FragCoord.xy / vec2(SCR_WIDTH, SCR_HEIGHT);
	
	Fragment frag;
	frag.position = texture(gPosition, texCoord).rgb;
	frag.normal = texture(gNormal, texCoord).rgb;
	vec4 albedoSpec = texture(gAlbedoSpec, texCoord);
	frag.albedo = albedoSpec.rgb;
	#ifdef ENABLE_SSAO
	frag.ambient = texture(ssaoTex, texCoord).r;
	#endif
	frag.specular = albedoSpec.a;
	frag.alphaMetalAo = texture(gAlphaMetalAo, texCoord).rgb;
	frag.F0 = mix(vec3(0.04), frag.albedo, frag.alphaMetalAo.g);
	frag.kDirect = pow((frag.alphaMetalAo.r + 1.0), 2.0) / 8.0;
	
	#ifdef IBL_PASS
	fragColor = vec4(CalcAmbient(frag), 1.0);
	#else
	fragColor = vec4(CalcLight(lights[lightIndex], frag), 1.0);
	#endif
	
	#ifdef ENABLE_BLOOM
	if (dot(vec3(0.2126, 0.7152, 0.0722), fragColor.rgb) > 1.0)
		brightColor = fragColor;
	else
		brightColor = vec4(vec3(0.0), 1.0);
	#endif //ENABLE_BLOOM
}