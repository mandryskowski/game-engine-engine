//in
in VS_OUT
{
	vec2 texCoord;
}	frag;

//out
out vec4 fragColor;

//uniform
uniform float gamma;
uniform sampler2D HDRbuffer;
#ifdef ENABLE_BLOOM
uniform sampler2D brightnessBuffer;
#endif

void main()
{
	vec3 HDRcolor = texture(HDRbuffer, frag.texCoord).rgb;
	
	#ifdef ENABLE_BLOOM
	vec3 brightColor = texture(brightnessBuffer, frag.texCoord).rgb;
	HDRcolor += brightColor;
	#endif
	
	HDRcolor /= HDRcolor + vec3(1.0);
	
	fragColor = vec4(pow(HDRcolor, vec3(1.0 / gamma)), 1.0);
	if (fragColor.rgb == vec3(0.0))
		discard;
	//fragColor = vec4(HDRcolor, 1.0);
}