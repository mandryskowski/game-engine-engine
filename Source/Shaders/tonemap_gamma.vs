layout (location = 0) in vec2 vPosition;
layout (location = 2) in vec2 vTexCoord;

//out
out VS_OUT
{
	vec2 texCoord;
}	vs_out;

void main()
{
	gl_Position = vec4(vPosition.xy, 0.0, 1.0);
	vs_out.texCoord = vTexCoord;
}