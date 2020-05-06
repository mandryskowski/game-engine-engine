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
uniform vec3 camPos;
uniform Material material;

#ifdef PREFIX_PARALLAX_OCCLUSION

vec2 ParallaxOcclusion(vec2 texCoord)	//Parallax Occlusion Mapping algorithm
{
	if (material.depthScale == 0.0)
		return texCoord;
	vec3 viewDir = transpose(frag.TBN) * normalize(camPos - frag.position);
	
	float minSamples = 8.0;
	float maxSamples = 16.0;
	
	float samples = mix(maxSamples, minSamples, abs(dot(viewDir, vec3(0.0, 0.0, 1.0))));
	float depthOffset = 1.0 / samples;
	vec2 unitOffset = viewDir.xy / viewDir.z * depthOffset * material.depthScale;
	
	texCoord -= unitOffset;
	
	for (float depth = depthOffset; depth < 1.0; depth += depthOffset)
	{
		float mapDepth = texture(material.depth1, texCoord).r;
		if (mapDepth < depth)
		{
			vec2 lastTexCoord = texCoord + unitOffset;
			float lastDepth = texture(material.depth1, lastTexCoord).r;
			float lastTestDepth = depth - depthOffset;
			
			float thisWeight = lastTestDepth - lastDepth;	//calculate delta from the last sample
			thisWeight = thisWeight / (thisWeight + (mapDepth - depth));	//divide the delta from the last sample by a sum of last delta and this sample delta
			
			return (texCoord * thisWeight) + (lastTexCoord * (1.0 - thisWeight));	//this way you can interpolate between texture coordinates to smoothen the result.
		}
		
		texCoord -= unitOffset;
	}
	
	return texCoord;
}

#endif

void main()
{
	vec2 texCoord = frag.texCoord;
	#ifdef PREFIX_PARALLAX_OCCLUSION
		//texCoord = ParallaxOcclusion(frag.texCoord);
	#endif
	vec3 normal = texture(material.normal1, texCoord).rgb;
	if (normal == vec3(0.0))
		normal = vec3(0.5, 0.5, 1.0);
	normal = normal * 2.0 - 1.0;
	normal = normalize(frag.TBN * normal);
	gPosition = frag.position;
	gNormal = normal;
	gAlbedoSpec.rgb = texture(material.diffuse1, texCoord).rgb;
	gAlbedoSpec.a = texture(material.specular1, texCoord).r;
}