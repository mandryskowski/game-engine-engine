layout (location = 0) in vec3 vPosition;

//out
out VS_OUT
{
	vec3 localPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 currMVPPosition;
	smooth vec4 prevMVPPosition;
	#endif
}	vs_out;

//uniform
uniform mat4 prevFlickerMat;
uniform mat4 flickerMat;
uniform mat4 VP;
#ifdef CALC_VELOCITY_BUFFER
uniform mat4 prevVP;
#endif

void main()
{
	vec4 projCoords = vec4(vec3(VP * vec4(vPosition, 0.0)), 1.0);
	gl_Position = projCoords.xyzz;
	vs_out.localPosition = vPosition;
	#ifdef CALC_VELOCITY_BUFFER
	vs_out.currMVPPosition = projCoords;
	vs_out.prevMVPPosition = flickerMat * vec4(vec3(prevVP * vec4(vPosition, 0.0)), 1.0);
	#endif
}