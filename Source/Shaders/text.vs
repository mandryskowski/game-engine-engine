layout (location = 0) in vec2 vPosition;
layout (location = 2) in vec2 vTexCoord;

//out
out vec2 texCoord;

//uniform
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vPosition + 0.5, 0.0, 1.0);
	texCoord = vTexCoord;
	texCoord = vPosition + 0.5;
}