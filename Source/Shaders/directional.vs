layout (location = 0) in vec2 vPosition;

void main()
{	
	gl_Position = vec4(vPosition * 2.0, 0.0, 1.0);
}