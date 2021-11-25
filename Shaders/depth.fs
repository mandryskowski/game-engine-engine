#ifdef MOUSE_PICKING
layout (location = 0) out uvec4 compID;

uniform vec4 compIDUniform;
void main()
{
	compID = uvec4(compIDUniform);
}
#elif SIMPLE_COLOR
layout (location = 0) out vec4 fragColor;

struct Material
{
	vec4 color;
};
uniform Material material;

void main()
{
	fragColor = material.color;
}
#elif OUTLINE_DISCARD_ALPHA
#define TEXTURE_ALPHA_DISCARD_THRESHOLD 0.5
struct Material
{
	sampler2D albedo1;
	vec4 color;
};

in vec2 texCoord;
out vec4 fragColor;
uniform Material material;

void main()
{
	if (texture(material.albedo1, texCoord).a < TEXTURE_ALPHA_DISCARD_THRESHOLD && material.color.a == 0.0)
		discard;
	fragColor = vec4(1.0);
}

#else	// OUTLINE_DISCARD_ALPHA
void main()
{
	;
}
#endif 