layout (location = 0) in vec3 vPosition;

//out
out vec3 localPosition;

//uniform
uniform mat4 VP;

void main()
{
	gl_Position = VP * vec4(vPosition, 1.0);
	localPosition = vPosition;
}