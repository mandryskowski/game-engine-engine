#version 400 core
layout (location = 0) in vec3 vPosition;

//out
out vec4 fragPos;

//uniform
uniform mat4 model;
uniform mat4 MVP;
layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};

void main()
{
	fragPos = model * vec4(vPosition, 1.0);
	gl_Position = MVP * vec4(vPosition, 1.0);
}