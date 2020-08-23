layout (location = 0) in vec3 vPosition;

//out
out VS_OUT
{
	vec3 localPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 prevMVPPosition;
	#endif
}	vs_out;

//uniform
uniform mat4 VP;
#ifdef CALC_VELOCITY_BUFFER
uniform mat4 prevVP;
#endif

void main()
{
	vec4 projCoords = VP * vec4(vPosition, 0.0);
	gl_Position = projCoords.xyzz;
	vs_out.localPosition = vPosition;
	#ifdef CALC_VELOCITY_BUFFER
	vs_out.prevMVPPosition = prevVP * vec4(vPosition, 0.0);
	#endif
}