//in
layout (location = 0) in vec2 vPosition;

//out
out vec2 currentPoint;

//uniform
uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(vPosition, 0.0, 1.0);
	currentPoint = vPosition * 0.5 + 0.5;	//from NDC to (0 -> 1)
}