#define MAX_LIGHTS 16

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

//in
in vec3 fragPosition;

//out
layout (location = 0) out vec4 fragColor;

//uniform
uniform int lightIndex;
uniform samplerCubeArray shadowCubemaps;
uniform sampler2DArray shadowMaps;
layout (std140) uniform Lights
{
	int lightCount;
	vec4 camPos;
	Light lights[MAX_LIGHTS];
};

void main()
{
	Light light = lights[lightIndex];
	#ifdef LIGHT_2D
	vec4 lightProj = light.lightSpaceMatrix * vec4(fragPosition, 1.0);
	vec3 lightCoords = lightProj.xyz /= lightProj.w;
	lightCoords = lightCoords * 0.5 + 0.5;
	fragColor = vec4(vec3(texture(shadowMaps, vec3(lightCoords.xy, light.shadowMapNr)).r), 1.0);
	#elif LIGHT_3D
	vec3 lightToFrag = fragPosition - light.position.xyz;
	fragColor = vec4(vec3(texture(shadowCubemaps, vec4(lightToFrag, light.shadowMapNr)).r), 1.0);
	#endif
}
