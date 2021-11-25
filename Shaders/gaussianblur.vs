layout (location = 0) in vec2 vPosition;
layout (location = 2) in vec2 vTexCoord;

//out
out vec2 texCoord;

void main()
{
	gl_Position = vec4(vPosition.x, vPosition.y, 0.0, 1.0);
	texCoord = vTexCoord;
}