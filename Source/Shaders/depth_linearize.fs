#version 400 core

//in
in vec4 fragPos;

//uniform
uniform float far;
uniform vec3 lightPos;


void main()
{
	float distanceFromLight = length(fragPos.xyz - lightPos);
	distanceFromLight /= far;
	gl_FragDepth = distanceFromLight;
}