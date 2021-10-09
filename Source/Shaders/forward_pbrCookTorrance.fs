#define MAX_LIGHTS 16
#define M_PI 3.14159265359
#define MAX_PREFILTER_MIPMAP 4.0

#define OLD_TYPE_POINT 1.0
#define OLD_TYPE_DIRECTIONAL 0.0
#define OLD_TYPE_SPOT 2.0

#if !defined(POINT_LIGHT) && !defined(DIRECTIONAL_LIGHT) && !defined(SPOT_LIGHT) && !defined(IBL_PASS)
#error Cannot compile Cook-Torrance shader without defining POINT_LIGHT, DIRECTIONAL_LIGHT, SPOT_LIGHT or IBL_PASS.
#endif

struct Light
{
	vec4 position;
	vec4 direction;
	
	vec4 ambientAndShadowBias;
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
};

struct Material
{
	sampler2D albedo1;
	sampler2D specular1;
	sampler2D normal1;
	
	vec4 color;
	bool disableColor;
	#ifdef ENABLE_POM
	sampler2D depth1;
	float depthScale;
	#endif
	#ifdef PBR_SHADING
	vec3 roughnessMetallicAoColor;
	
	sampler2D roughness1;
	sampler2D metallic1;
	sampler2D ao1;
	sampler2D combined1;
	#endif
	
	float shininess;
};
//in
in VS_OUT
{
	vec3 worldPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 currMVPPosition;
	smooth vec4 prevMVPPosition;
	#endif
	
	vec2 texCoord;
	
	mat3 TBN;
}	fragIn;

//out
layout (location = 0) out vec4 fragColor;
#ifdef ENABLE_BLOOM
layout (location = 1) out vec4 brightColor;
#endif

//uniform
uniform Material material;
uniform int lightIndex;

uniform samplerCubeArray shadowCubemaps;
uniform sampler2DArray shadowMaps;

layout (std140) uniform Lights
{
	int lightCount;
	vec4 camPos;
	Light lights[MAX_LIGHTS];
};


////////////////////////////////
////////////////////////////////
////////////////////////////////

//#ifdef POINT_LIGHT
float CalcShadow3D(Light light, vec3 fragPosition)
{
	vec3 lightToFrag = fragPosition - light.position.xyz;

	return ((length(lightToFrag) / light.far > texture(shadowCubemaps, vec4(lightToFrag, light.shadowMapNr)).r + light.ambientAndShadowBias.a) ? (1.0) : (0.0));
}
//#else
float CalcShadow2D(Light light, vec3 fragPosition)
{
	vec4 lightProj = light.lightSpaceMatrix * vec4(fragPosition, 1.0);
	vec3 lightCoords = lightProj.xyz /= lightProj.w;
	lightCoords = lightCoords * 0.5 + 0.5;
	
	return (lightCoords.z > texture(shadowMaps, vec3(lightCoords.xy, light.shadowMapNr)).r + light.ambientAndShadowBias.a) ? (1.0) : (0.0);
}
//#endif

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
	return F0 + (1.0 - F0) * pow(max(1.0 - HdotV, 0.0), 5.0);	//Note: 1.0 - HdotV must be positive (or 0) because pow() is undefined if the first argument is negative. Due to precision errors, HdotV might actually be a bit larger than 1.
}

vec3 FresnelSchlickRoughness(float HdotV, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - HdotV, 0.0), 5.0);
}

vec3 CalcLight(Light light, Fragment frag)
{
	vec3 l = normalize(light.position.xyz - frag.position);
	if(light.type == OLD_TYPE_DIRECTIONAL)
		vec3 l = normalize(-light.direction.xyz);

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
	
	vec3 radiance = vec3(0.0);
	vec3 ambient = vec3(0.0);
	
	if(light.type == OLD_TYPE_DIRECTIONAL)
	{
		float visibility = 1.0 - CalcShadow2D(light, frag.position);
		
		radiance = light.diffuse.rgb * visibility;
		ambient = light.ambientAndShadowBias.rgb * frag.albedo * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * (1 - Metalness)
	}
	else if(light.type == OLD_TYPE_SPOT)
	{
		float visibility = 1.0 -  CalcShadow2D(light, frag.position);
		
		float dist = length(light.position.xyz - frag.position);
		float attenuation = 1.0 / max((dist * dist * light.attenuation), 0.001);
		
		float LdotLDir = max(dot(l, -light.direction.xyz), 0.0);
		float spotIntensity = clamp((LdotLDir - light.outerCutOff) / max((light.cutOff - light.outerCutOff), 0.001), 0.0, 1.0);
		
		radiance = light.diffuse.rgb * attenuation * spotIntensity * visibility;
		ambient = light.ambientAndShadowBias.rgb * frag.albedo * attenuation * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * Attenuation * (1 - Metalness)
	}
	else
	{
		float visibility = 1.0 - CalcShadow3D(light, frag.position);
		
		float dist = length(light.position.xyz - frag.position);
		float attenuation = 1.0 / (dist * dist * light.attenuation);
		
		radiance = light.diffuse.rgb * attenuation * visibility;
		ambient = light.ambientAndShadowBias.rgb * frag.albedo * attenuation * (1.0 - frag.alphaMetalAo.g);	//Ambient * Albedo * Attenuation * (1 - Metalness)
	}
	
	
	return ((kDiffuse * frag.albedo / M_PI) + ((D * F * G) / (4.0 * NdotL * NdotV))) * radiance * NdotL + ambient;
}

Fragment getFragment()
{
	Fragment frag;
	vec2 texCoord = fragIn.texCoord;
	
	vec3 normal = texture(material.normal1, texCoord).rgb;
	if (normal == vec3(0.0))
		normal = vec3(0.5, 0.5, 1.0);
	normal = normal * 2.0 - 1.0;
	normal = normalize(fragIn.TBN * normal);
	

	frag.position = fragIn.worldPosition;
	frag.normal = normal;
	frag.albedo.rgb = texture(material.albedo1, texCoord).rgb;
	if (!material.disableColor && frag.albedo.rgb == vec3(0.0))
		frag.albedo.rgb = material.color.rgb;
	
	frag.specular = texture(material.specular1, texCoord).r;
	//#ifdef PBR_SHADING
	frag.alphaMetalAo.r = texture(material.roughness1, texCoord).r;
	if (frag.alphaMetalAo.r == 0.0)	frag.alphaMetalAo.r = texture(material.combined1, texCoord).g;
	if (frag.alphaMetalAo.r == 0.0)	frag.alphaMetalAo.r = material.roughnessMetallicAoColor.r;
	frag.alphaMetalAo.r = pow(frag.alphaMetalAo.r, 2.0);
	
	frag.alphaMetalAo.g = texture(material.metallic1, texCoord).r;
	if (frag.alphaMetalAo.g == 0.0)	frag.alphaMetalAo.g = texture(material.combined1, texCoord).b;
	if (frag.alphaMetalAo.g == 0.0) frag.alphaMetalAo.g = material.roughnessMetallicAoColor.g;
	
	frag.alphaMetalAo.b = texture(material.ao1, texCoord).r;
	if (frag.alphaMetalAo.b == 0.0) frag.alphaMetalAo.b = material.roughnessMetallicAoColor.b;
	
	//#endif
	return frag;
}


void main() 
{
	vec2 texCoord = gl_FragCoord.xy / vec2(SCR_WIDTH, SCR_HEIGHT);
	
	Fragment frag = getFragment();

	frag.F0 = mix(vec3(0.04), frag.albedo, frag.alphaMetalAo.g);
	frag.kDirect = pow((frag.alphaMetalAo.r + 1.0), 2.0) / 8.0;
	
	fragColor = vec4(vec3(0.0), 1.0);
	#ifdef ENABLE_BLOOM
	brightColor = vec4(vec3(0.0), 1.0);
	#endif
	for (int i = 0; i < lightCount; i++)
	{
		vec3 thisLightColor = CalcLight(lights[i], frag);
		fragColor.rgb += thisLightColor;
		#ifdef ENABLE_BLOOM
		if (dot(vec3(0.2126, 0.7152, 0.0722), thisLightColor.rgb) > 1.0)
			brightColor.rgb += thisLightColor;
		#endif
	}
}