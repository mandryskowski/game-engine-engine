//in
in vec2 texCoord;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 blurColor;

//uniform
uniform int glyphNr;
uniform vec3 color;
uniform sampler2DArray glyphsTex;

#ifdef MOUSE_PICKING
uniform uvec4 compIDUniform;
#endif //  MOUSE_PICKING

void main()
{
	float alpha = texture(glyphsTex, vec3(vec2(texCoord.x, 1.0 - texCoord.y), glyphNr)).r;
	if (alpha == 0.0)
		discard;
		
	#ifdef MOUSE_PICKING
	fragColor = vec4(compIDUniform);
	#else
	fragColor = vec4(color, alpha);
	blurColor = vec4(0.0);
	#endif
}