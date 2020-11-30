struct Material
{
	vec4 color;
	sampler2D albedo1;
};

//in
in vec2 texCoord1;
in vec2 texCoord2;
in float blend;

//out
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

//uniform
uniform Material material;


void main()
{
	fragColor = mix(texture(material.albedo1, texCoord1), texture(material.albedo1, texCoord2), blend);
	if (material.color != vec4(0.0))
		fragColor = material.color;
	brightColor = fragColor;
	
	//fragColor = vec4(texCoord1, 0.0, 1.0);
	
	if (fragColor.a < 0.5)
		discard;
		
	fragColor.rgb *= fragColor.rgb + 1.0;
}
