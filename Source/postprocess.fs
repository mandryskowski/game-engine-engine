#version 400 core

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
uniform sampler2D brightnessBuffer;

void main()
{
	vec3 HDRcolor = texture(HDRbuffer, frag.texCoord).rgb;
	vec3 brightColor = texture(brightnessBuffer, frag.texCoord).rgb;
	
	fragColor = vec4(HDRcolor, 1.0);
	//return;
	
	HDRcolor += brightColor;
	HDRcolor /= HDRcolor + vec3(1.0);
	
	fragColor = vec4(pow(HDRcolor, vec3(1.0 / gamma)), 1.0);
}