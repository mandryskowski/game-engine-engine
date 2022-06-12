//#define SOFT_SHADOWS 1
//in
in vec4 fragPos;

//uniform
uniform float far;
#ifdef SOFT_SHADOWS
uniform float lightBias;
#endif
uniform vec3 lightPos;


void main()
{
	#ifdef SOFT_SHADOWS
	float distanceFromLight = length(fragPos.xyz - lightPos) + ((gl_FrontFacing) ? (lightBias) : (0.0));
	#else // SOFT_SHADOWS
	float distanceFromLight = length(fragPos.xyz - lightPos);
	#endif
	distanceFromLight /= far;
	gl_FragDepth = distanceFromLight;
}