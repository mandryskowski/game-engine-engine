//in
in vec2 texCoord;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 blurColor;

//uniform
uniform int glyphNr;
uniform vec3 color;
uniform sampler2DArray glyphsTex;

void main()
{
	float alpha = texture(glyphsTex, vec3(vec2(texCoord.x, 1.0 - texCoord.y), glyphNr)).r;
	if (alpha == 0.0)
		discard;
		
	fragColor = vec4(pow(color, vec3(2.2)), alpha);
	blurColor = vec4(0.0);
}