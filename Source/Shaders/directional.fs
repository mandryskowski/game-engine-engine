#define MAX_LIGHTS 16
#define ENABLE_BLOOM

struct Light
{
	vec4 position;
	vec4 direction;
	
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	
	float attenuation;	//quadratic component of light's attenuation equation
	float cutOff;		//cosine of the angle of the inner fallof cone (spot lights)
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
layout(location = 0) out vec4 fragColor;
#ifdef ENABLE_BLOOM
layout(location = 1) out vec4 brightColor;
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

float CalcShadow2D(Light light, vec3 fragPosition)
{
	vec4 projCoords = light.lightSpaceMatrix * vec4(fragPosition, 1.0);
	vec3 fragCoord = projCoords.xyz / projCoords.w;
	fragCoord = fragCoord / 2.0 + 0.5;
	
	float mapDepth = texture(shadowMaps, vec3(fragCoord.xy, light.shadowMapNr)).r;
	
	return ((fragCoord.z > mapDepth) ? (1.0) : (0.0));
}

vec3 CalcLight(Light light, Fragment frag)
{
	vec3 lightDir = normalize(-light.direction.xyz);;

	//////////////
	#ifdef ENABLE_SSAO
	vec3 ambient = light.ambient.rgb * frag.albedo * frag.ambient;
	#endif
	#ifndef ENABLE_SSAO
	vec3 ambient = light.ambient.rgb * frag.albedo;
	#endif
	
	
	float diff = max(dot(frag.normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse.rgb * frag.albedo * diff;
	
	vec3 camDir = normalize(camPos.xyz - frag.position);
	vec3 halfwayDir = normalize(camDir + lightDir);
	float spec = pow(max(dot(frag.normal, halfwayDir), 0.0), 64);
	vec3 specular = light.specular.rgb * frag.specular * spec;
	
	float shadow = 0.0;
	shadow = CalcShadow2D(light, frag.position);
	
	if (shadow == 1.0)
		return ambient;

	return ambient + diffuse + specular;
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