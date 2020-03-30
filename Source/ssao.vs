#version 400 core
layout (location = 0) in vec2 vPosition;

out vec2 texCoord;

void main()
{
	texCoord = vPosition * 0.5 + 0.5;
	gl_Position = vec4(vPosition, 0.0, 1.0);
}