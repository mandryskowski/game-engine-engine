#version 400 core
layout (location = 0) in vec3 vPosition;

uniform mat4 model;
layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};
void main()
{
	gl_Position = projection * view * model * vec4(vPosition, 1.0);
}