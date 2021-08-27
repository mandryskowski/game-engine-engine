//in
in vec2 texCoord;

//out
out vec4 fragColor;

//uniform
uniform sampler2D tex;
#ifdef OUTLINE_FROM_SILHOUETTE
struct Material
{
	vec4 color;
};
uniform Material material;
#endif


void main()
{
	#ifdef OUTLINE_FROM_SILHOUETTE	// Renders an outline around the silhouette contained in the "tex" texture.
	
	#define OUTLINE_THICKNESS 2.0
	
	vec2 unitOffset = 1.0 / textureSize(tex, 0);
	
	if (texture(tex, texCoord).rgb != vec3(0.0))	// if at silhouette, discard - do not draw anything
		discard;
	
	for (float y = -OUTLINE_THICKNESS; y <= OUTLINE_THICKNESS; y++)
	{
		for (float x = -OUTLINE_THICKNESS; x <= OUTLINE_THICKNESS; x++)
		{
			if (x == 0 && y == 0)
				continue;
			if (texture(tex, texCoord + vec2(x, y) * unitOffset).rgb != vec3(0.0))	// if next to silhouette border
			{
				fragColor = material.color;
				return;
			}
		}
	}
	discard;	// if not next to silhouette border, discard
	#else	// OUTLINE_FROM_SILHOUETTE
	fragColor = texture(tex, texCoord);
	#endif
}