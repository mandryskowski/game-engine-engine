#version 400 core
layout (location = 0) in vec3 vPosition;

uniform mat4 model;
uniform mat4 MVP;
layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};
void main()
{
	gl_Position = MVP * vec4(vPosition, 1.0);
}