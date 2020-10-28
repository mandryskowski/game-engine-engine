struct Material
{
	sampler2D albedo1;
	sampler2D specular1;
	sampler2D normal1;
	#ifdef ENABLE_POM
	sampler2D depth1;
	float depthScale;
	#endif
	#ifdef PBR_SHADING
	sampler2D roughness1;
	sampler2D metallic1;
	sampler2D ao1;
	#endif
	
	float shininess;
};

//in
in VS_OUT
{
	vec3 worldPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 prevMVPPosition;
	#endif
	
	vec2 texCoord;
	
	mat3 TBN;
}	frag;

//out
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;
#ifdef PBR_SHADING
layout (location = 3) out vec3 gAlphaMetalAo;
#endif
#if defined(CALC_VELOCITY_BUFFER) && defined(PBR_SHADING)
layout (location = 4) out vec2 velocity;
#elif defined(CALC_VELOCITY_BUFFER)
layout (location = 3) out vec2 velocity;
#endif

//uniform
uniform float velocityScale;
uniform vec3 camPos;
uniform Material material;

#ifdef ENABLE_POM

vec2 ParallaxOcclusion(vec2 texCoord)	//Parallax Occlusion Mapping algorithm
{
	if (material.depthScale == 0.0)
		return texCoord;

	vec3 viewDir = transpose(frag.TBN) * normalize(camPos - frag.worldPosition);
	
	#if defined(POM_PRESET_LOW)
	float minSamples = 8.0;
	float maxSamples = 16.0;
	#elif defined(POM_PRESET_MEDIUM)
	float minSamples = 16.0;
	float maxSamples = 24.0;
	#elif defined(POM_PRESET_HIGH)
	float minSamples = 32.0;
	float maxSamples = 64.0;
	#elif defined(POM_PRESET_ULTRA)
	float minSamples = 64.0;
	float maxSamples = 128.0;
	#else
	#error No POM preset defined.
	#endif
	
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
	#ifdef ENABLE_POM
	texCoord = ParallaxOcclusion(frag.texCoord);
	#endif
	
	vec3 normal = texture(material.normal1, texCoord).rgb;
	if (normal == vec3(0.0))
		normal = vec3(0.5, 0.5, 1.0);
	normal = normal * 2.0 - 1.0;
	normal = normalize(frag.TBN * normal);
	
	gPosition = frag.worldPosition;
	gNormal = normal;
	gAlbedoSpec.rgb = texture(material.albedo1, texCoord).rgb;
	gAlbedoSpec.a = texture(material.specular1, texCoord).r;
	#ifdef PBR_SHADING
	gAlphaMetalAo.r = pow(texture(material.roughness1, texCoord).r, 2.0);
	gAlphaMetalAo.g = texture(material.metallic1, texCoord).r;
	gAlphaMetalAo.b = texture(material.ao1, texCoord).r;
	#endif
	
	#ifdef CALC_VELOCITY_BUFFER
	vec2 currentPos = vec2(gl_FragCoord.xy) / vec2(SCR_WIDTH, SCR_HEIGHT);
	vec2 previousPos = (frag.prevMVPPosition.xy / frag.prevMVPPosition.w) * 0.5 + 0.5;
	velocity = (currentPos - previousPos);
	#endif
}