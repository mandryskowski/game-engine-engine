//in
in vec3 vertColor;

//out
layout (location = 0) out vec4 fragColor;

//uniform
uniform vec3 color;

void main()
{
	if(vertColor != vec3(0.0))
		fragColor = vec4(vertColor, 1.0);
	else
		fragColor = vec4(color, 1.0);
}