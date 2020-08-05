//in
in vec2 texCoord;

//out
out vec4 fragColor;

//uniform
uniform sampler2D tex;

void main()
{
	fragColor = texture(tex, texCoord);
}