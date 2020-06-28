layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;

//out
out vec3 vertColor;

//uniform
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vPosition, 1.0);
	vertColor = vColor;
}