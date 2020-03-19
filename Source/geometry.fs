#version 400 core

struct Material
{
	sampler2D diffuse1;
	sampler2D specular1;
	sampler2D normal1;
	sampler2D depth1;
	
	float shininess;
	float depthScale;
};

//in
in VS_OUT
{
	vec3 position;
	vec2 texCoord;
	
	mat3 TBN;
}	frag;

//out
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

//uniform
uniform Material material;

void main()
{
	vec3 normal = texture(material.normal1, frag.texCoord).rgb;
	if (normal == vec3(0.0))
		normal = vec3(0.5, 0.5, 1.0);
	normal = normal * 2.0 - 1.0;
	normal = normalize(frag.TBN * normal);
	gPosition = frag.position;
	gNormal = normal;
	gAlbedoSpec.rgb = texture(material.diffuse1, frag.texCoord).rgb;
	gAlbedoSpec.a = texture(material.specular1, frag.texCoord).r;
}