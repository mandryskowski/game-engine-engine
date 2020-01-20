#version 330 core
layout (location = 0) in vec2 vPosition;
layout (location = 1) in vec2 vTexCoord;

//out
out VS_OUT
{
	vec2 texCoord;
}	vs_out;

void main()
{
	gl_Position = vec4(vPosition, 0.0, 1.0);
	vs_out.texCoord = vTexCoord;
}