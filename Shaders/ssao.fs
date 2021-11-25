//in
in vec2 texCoord;

//out
out float ambientOcclusion;

//uniform
uniform float radius;
uniform vec3 samples[SSAO_SAMPLES];
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseTex;

void main()
{
	vec2 noiseTexCoordScale = vec2(SCR_WIDTH, SCR_HEIGHT) / vec2(4.0);
	
	vec3 fragPos = vec3(view * vec4(texture(gPosition, texCoord).xyz, 1.0));
	vec3 fragNormal = normalize(mat3(view) * texture(gNormal, texCoord).xyz);
	vec3 randomVec = normalize(texture(noiseTex, texCoord * noiseTexCoordScale).xyz);
	
	vec3 tangent = normalize(randomVec - fragNormal * dot(fragNormal, randomVec));
	vec3 bitangent = cross(fragNormal, tangent);
	
	mat3 TBN = mat3(tangent, bitangent, fragNormal);
	
	ambientOcclusion = 0.0;
	
	for (int i = 0; i < SSAO_SAMPLES; i++)
	{
		vec3 thisSample = TBN * samples[i];
		thisSample = fragPos + thisSample * radius;
		
		vec4 projCoords = projection * vec4(thisSample, 1.0);
		projCoords.xyz /= projCoords.w;
		projCoords = projCoords * 0.5 + 0.5;
		
		vec3 samplePos = texture(gPosition, projCoords.xy).xyz;
		if (samplePos == vec3(0.0))
			continue;
		float sampleDepth = (view * vec4(samplePos, 1.0)).z;
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		ambientOcclusion += (sampleDepth > thisSample.z + 0.025) ? (rangeCheck) : (0.0);
	}
	
	ambientOcclusion = 1.0 - ambientOcclusion / SSAO_SAMPLES;
}