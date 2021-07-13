//in
in vec2 texCoord;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 blurColor;

struct Material
{
	vec4 color;
	sampler2DArray glyphsTex;	// contains all the glyphs of the font
	int glyphNr;
};

//uniform
uniform Material material;

void main()
{
	float alpha = texture(material.glyphsTex, vec3(vec2(texCoord.x, 1.0 - texCoord.y), material.glyphNr)).r;
	if (alpha == 0.0)
		discard;
		
	fragColor = vec4(material.color.rgb, alpha * material.color.a);
	blurColor = vec4(0.0);
}