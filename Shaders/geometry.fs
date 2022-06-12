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
	bool bHasTangents;
	vec3 normalWithoutNormalMap;
}	frag;

//out
layout (location = 0) out vec4 gAlbedoSpec;
layout (location = 1) out vec3 gPosition;
layout (location = 2) out vec3 gNormal;
#ifdef PBR_SHADING
layout (location = 3) out vec3 gAlphaMetalAo;
#endif
#if defined(CALC_VELOCITY_BUFFER) && defined(PBR_SHADING)
layout (location = 4) out vec2 velocity;
#elif defined(CALC_VELOCITY_BUFFER)
layout (location = 3) out vec2 velocity;
#endif

//uniform
uniform vec3 camPos;
uniform Material material;

#ifdef ENABLE_POM

vec2 ParallaxOcclusion(vec2 texCoord)	//Parallax Occlusion Mapping algorithm
{
	if (material.depthScale == 0.0)	//this won't optimize anything but rather prevent any unnecessary texCoord change
		return texCoord;

	vec3 viewDir = normalize(transpose(frag.TBN) * normalize(camPos - frag.worldPosition));
	
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
	#error No POM preset defined. Define POM_PRESET_LOW/POM_PRESET_MEDIUM/POM_PRESET_HIGH/POM_PRESET_ULTRA.
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
			thisWeight = thisWeight / (thisWeight + (mapDepth - depth));	//divide the delta from the last sample by the sum of last delta and this sample delta
			
			return mix(lastTexCoord, texCoord, thisWeight);	//this way we can interpolate between texture coordinates to smoothen the result.
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
	
	vec3 normal = frag.normalWithoutNormalMap;
	//if (frag.bHasTangents)	// Note: This "if" is not an optimisation; the code inside it will run anyways, due to the parallel nature of GPUs.
	{
		normal = texture(material.normal1, texCoord).rgb;
		if (normal == vec3(0.0))
			normal = vec3(0.5, 0.5, 1.0);
		normal = normal * 2.0 - 1.0;
		normal = normalize(frag.TBN * normal);
	}
	

	gPosition = frag.worldPosition;
	gNormal = normal;
	gAlbedoSpec.rgb = texture(material.albedo1, texCoord).rgb;
	if (!material.disableColor && gAlbedoSpec.rgb == vec3(0.0))
		gAlbedoSpec.rgb = material.color.rgb;
	//else if (texture(material.albedo1, texCoord).a < 0.5)
		//discard;
	gAlbedoSpec.a = texture(material.specular1, texCoord).r;
	#ifdef PBR_SHADING
	gAlphaMetalAo.r = texture(material.roughness1, texCoord).r;
	if (gAlphaMetalAo.r == 0.0)	gAlphaMetalAo.r = texture(material.combined1, texCoord).g;
	if (gAlphaMetalAo.r == 0.0)	gAlphaMetalAo.r = material.roughnessMetallicAoColor.r;
	gAlphaMetalAo.r = pow(gAlphaMetalAo.r, 2.0);
	
	gAlphaMetalAo.g = texture(material.metallic1, texCoord).r;
	if (gAlphaMetalAo.g == 0.0)	gAlphaMetalAo.g = texture(material.combined1, texCoord).b;
	if (gAlphaMetalAo.g == 0.0) gAlphaMetalAo.g = material.roughnessMetallicAoColor.g;
	
	gAlphaMetalAo.b = texture(material.ao1, texCoord).r;
	if (gAlphaMetalAo.b == 0.0) gAlphaMetalAo.b = material.roughnessMetallicAoColor.b;
	
	#endif
	
	#ifdef CALC_VELOCITY_BUFFER
	//vec2 currentPos = vec2(gl_FragCoord.xy) / vec2(SCR_WIDTH, SCR_HEIGHT);
	vec2 currentPos = (frag.currMVPPosition.xy / frag.currMVPPosition.w) * 0.5 + 0.5;
	vec2 previousPos = (frag.prevMVPPosition.xy / frag.prevMVPPosition.w) * 0.5 + 0.5;
	velocity = (currentPos - previousPos);
	#endif
}