#version 330 core


//out
layout (location = 0) out vec4 fragColor;

//uniform
uniform vec3 color;

void main()
{
	fragColor = vec4(color, 1.0);
}