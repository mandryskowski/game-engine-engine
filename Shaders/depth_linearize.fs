//in
in vec4 fragPos;

//uniform
uniform float far;
uniform float lightBias;
uniform vec3 lightPos;


void main()
{
	float distanceFromLight = length(fragPos.xyz - lightPos) + ((gl_FrontFacing) ? (lightBias) : (0.0));
	distanceFromLight /= far;
	gl_FragDepth = distanceFromLight;
}