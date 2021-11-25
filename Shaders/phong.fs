#define MAX_LIGHTS 16

#if !defined(POINT_LIGHT) && !defined(DIRECTIONAL_LIGHT) && !defined(SPOT_LIGHT)
#error Cannot compile Phong shader without defining POINT_LIGHT, DIRECTIONAL_LIGHT or SPOT_LIGHT.
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
	#ifdef ENABLE_SSAO
	float ambient;
	#endif
	float specular;
};

//out
layout (location = 0) out vec4 fragColor;
#ifdef ENABLE_BLOOM
layout (location = 1) out vec4 brightColor;
#endif

//uniform
uniform int lightIndex;
uniform vec2 resolution;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
#ifdef ENABLE_SSAO
uniform sampler2D ssaoTex;
#endif
#ifdef POINT_LIGHT
uniform samplerCubeArray shadowCubemaps;
#else
uniform sampler2DArray shadowMaps;
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

vec3 CalcLight(Light light, Fragment frag)
{
	#ifdef DIRECTIONAL_LIGHT
	vec3 l = normalize(-light.direction.xyz);
	#else
	vec3 l = normalize(light.position.xyz - frag.position);
	#endif

	//////////////
	
	#ifdef ENABLE_SSAO
	vec3 ambient = light.ambient.rgb * frag.albedo * frag.ambient;
	#else
	vec3 ambient = light.ambient.rgb * frag.albedo;
	#endif
	
	float diff = max(dot(frag.normal, l), 0.0);
	vec3 diffuse = light.diffuse.rgb * frag.albedo * diff;
	
	vec3 v = normalize(camPos.xyz - frag.position);
	vec3 h = normalize(v + l);
	float spec = pow(max(dot(frag.normal, h), 0.0), 64);
	vec3 specular = light.specular.rgb * frag.specular * spec;
	
	#if defined(POINT_LIGHT) || defined(SPOT_LIGHT)
	float attenuation = 1.0;
	if (light.attenuation != 0.0)
	{
		float dist = distance(frag.position, light.position.xyz);
		attenuation = 1.0 / (dist * dist * light.attenuation);
	}
	#endif
	
	#ifdef POINT_LIGHT
	float shadow = CalcShadow3D(light, frag.position);
	#else
	float shadow = CalcShadow2D(light, frag.position);
	#endif

	#if defined(POINT_LIGHT)
	if (shadow == 1.0)
		return ambient * attenuation;
	
	
	return (ambient + diffuse + specular) * attenuation;
	#elif defined(SPOT_LIGHT)
	float spotAlignment = max(dot(-light.direction.xyz, l), 0.0);
	float intensity = (spotAlignment - light.outerCutOff) / (light.cutOff - light.outerCutOff);
	if (shadow == 1.0)
		return ambient * attenuation;
	
	
	return (ambient + (diffuse + specular) * intensity) * attenuation;
	#else
	if (shadow == 1.0)
		return ambient;
		
	
	return ambient + diffuse + specular;
	#endif
	
}

void main() 
{
	vec2 texCoord = gl_FragCoord.xy / resolution;
	
	Fragment frag;
	frag.position = texture(gPosition, texCoord).rgb;
	frag.normal = texture(gNormal, texCoord).rgb;
	vec4 albedoSpec = texture(gAlbedoSpec, texCoord);
	frag.albedo = albedoSpec.rgb;
	#ifdef ENABLE_SSAO
	frag.ambient = texture(ssaoTex, texCoord).r;
	#endif
	frag.specular = albedoSpec.a;
	
	fragColor = vec4(CalcLight(lights[lightIndex], frag), 1.0);
	
	#ifdef ENABLE_BLOOM
	if (dot(vec3(0.2126, 0.7152, 0.0722), fragColor.rgb) > 1.0)
		brightColor = fragColor;
	else
		brightColor = vec4(vec3(0.0), 1.0);
	#endif //ENABLE_BLOOM
}