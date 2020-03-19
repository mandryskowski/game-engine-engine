#version 400 core
layout (location = 0) in vec3 vPosition;

//uniform
uniform mat4 model;
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vPosition, 1.0);
}