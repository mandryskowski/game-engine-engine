#version 400 core
#define MAX_LIGHTS 16
#define TYPE_DIR 0
#define TYPE_POINT 1
#define TYPE_SPOT 2

struct Material
{
	sampler2D diffuse1;
	sampler2D specular1;
	sampler2D normal1;
	sampler2D depth1;
	
	float shininess;
	float depthScale;
};

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
	int type;			//determines the type of the light - directional, point or spot
	
	int shadowMapNr;
	float far;				//used for lights that use Ominidirectional Shadow Mapping (point lights)
	mat4 lightSpaceMatrix;	//used for lights that use 2D shadowmaps (directional and spot lights)
};

//in
in VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	vec3 tangentDupa;
	
	mat3 TBN;
}	frag;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

//uniforms
uniform sampler2DArray shadowMaps;
uniform samplerCubeArray shadowCubeMaps;
uniform Material material;
layout (std140) uniform Lights
{
	int lightCount;
	vec4 camPos;
	Light lights[MAX_LIGHTS];
};

//functions

vec2 ParallaxOcclusion(vec2 texCoord)
{
	if (material.depthScale == 0.0)
		return texCoord;
	

	vec3 viewDir = transpose(frag.TBN) * normalize(camPos.xyz - frag.position);
	
	float minSamples = 8.0;
	float maxSamples = 16.0;
	float numSamples = mix(maxSamples, minSamples, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
	
	float depthOffset = 1.0 / numSamples;
	vec2 unitOffset = viewDir.xy / viewDir.z * depthOffset * material.depthScale;
	texCoord -= unitOffset;
	
	for (float depth = depthOffset; depth < 1.0; depth += depthOffset)
	{
		float sampleDepth = texture(material.depth1, texCoord).r;
		
		if (depth > sampleDepth)
		{
			vec2 prevTexCoord = texCoord + unitOffset;
			float prevDepth = depth - depthOffset;
			float prevSampleDepth = texture(material.depth1, prevTexCoord).r;
			
			float deltaAfter = depth - sampleDepth;
			float deltaBefore = prevSampleDepth - prevDepth;
			
			float weight = deltaAfter / (deltaAfter + deltaBefore);
			
			texCoord = texCoord * (1.0 - weight) + prevTexCoord * weight;
			break;
		}
		
		texCoord -= unitOffset;
	}
	
	return texCoord;
}

float CalcShadow2D(Light light)
{
	vec4 lightSpacePos = light.lightSpaceMatrix * vec4(frag.position, 1.0);
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;	//returns light space position in range [-1, 1]
	projCoords = projCoords * 0.5 + 0.5;	//map it to range [0, 1]
	
	float mapDepth = texture(shadowMaps, vec3(projCoords.xy, light.shadowMapNr)).r;
	float actualDepth = projCoords.z;
	
	if (actualDepth > 1.0)
		return 0.0;
	
	return (actualDepth > mapDepth + 0.005) ? (1.0) : (0.0);
}

float CalcShadow3D(Light light)
{
	vec3 lightToFrag = frag.position - light.position.xyz;
	float mapDepth = texture(shadowCubeMaps, vec4(lightToFrag, light.shadowMapNr)).r;
	if (mapDepth > 1.0)
		return 0.0;
	float actualDepth = length(lightToFrag) / light.far;

	return (actualDepth > mapDepth + 0.02) ? (1.0) : (0.0);
}

vec3 CalcLight(Light light, vec3 diffuseColor, vec3 specularColor, vec3 normal)
{
	vec3 lightDir = normalize(light.position.rgb - frag.position);;
	if (light.type == TYPE_DIR)
		lightDir = normalize(-light.direction.rgb);

	//////////////
	
	vec3 ambient = light.ambient.rgb * diffuseColor;
	
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse.rgb * diffuseColor * diff;
	
	vec3 camDir = normalize(camPos.xyz - frag.position);
	vec3 halfwayDir = normalize(camDir + lightDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
	vec3 specular = light.specular.rgb * specularColor * spec;
	
	float attenuation = 1.0;
	if (light.attenuation != 0.0)
	{
		float dist = distance(frag.position, light.position.xyz);
		attenuation = 1.0 / (dist * dist * light.attenuation);
	}
	
	float intensity = 1.0;
	if (light.type == TYPE_SPOT)
	{
		float alignmentWithSpotDir = dot(-lightDir, normalize(light.direction.xyz));
		intensity = clamp((alignmentWithSpotDir - light.outerCutOff) / (light.cutOff - light.outerCutOff), 0.0, 1.0);
	}
	
	float shadow = 0.0;
	
	if (light.type == TYPE_POINT)
		shadow = CalcShadow3D(light);
	else
		shadow = CalcShadow2D(light);
	
	if (shadow == 1.0)
		return ambient * attenuation;
	
	return ((diffuse + specular) * intensity + ambient) * attenuation;
}

////////////////////////////

void main()
{
	fragColor = vec4(1.0);
	//return;
	
	vec2 parallaxCoord = ParallaxOcclusion(frag.texCoord);
	vec3 diffuseColor = texture(material.diffuse1, parallaxCoord).rgb;
	vec3 specularColor = texture(material.specular1, parallaxCoord).rgb;
	vec3 normal = texture(material.normal1, parallaxCoord).rgb;
	if (normal == vec3(0.0))
		normal = vec3(0.5, 0.5, 1.0);
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(frag.TBN * normal);

	fragColor = vec4(vec3(0.0), 1.0);
	for (int i = 0; i < lightCount; i++)
		fragColor.rgb += CalcLight(lights[i], diffuseColor, specularColor, normal);
	
	
	float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if (brightness > 1.0)
		brightColor = vec4(fragColor.rgb, 1.0);
	else
		brightColor = vec4(0.0, 0.0, 0.0, 1.0);
}