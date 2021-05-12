#define VELOCITY_BUFFER_LOCATION 2
//in
in VS_OUT
{
	vec3 localPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 currMVPPosition;
	smooth vec4 prevMVPPosition;
	#endif
}	frag;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;
#ifdef CALC_VELOCITY_BUFFER
layout (location = VELOCITY_BUFFER_LOCATION) out vec2 velocity;
#endif

//uniform
uniform float cubemapNr;
uniform float mipLevel;
uniform samplerCube cubemap;

void main()
{
	fragColor = textureLod(cubemap, vec3(frag.localPosition), 0.0);
	
	#ifdef CALC_VELOCITY_BUFFER
	//vec2 currentPos = vec2(gl_FragCoord.xy) / vec2(SCR_WIDTH, SCR_HEIGHT);
	vec2 currentPos = (frag.currMVPPosition.xy / frag.currMVPPosition.w) * 0.5 + 0.5;
	vec2 previousPos = (frag.prevMVPPosition.xy / frag.prevMVPPosition.w) * 0.5 + 0.5;
	velocity = (currentPos - previousPos);
	#endif
	
	brightColor = vec4(0.0);
}