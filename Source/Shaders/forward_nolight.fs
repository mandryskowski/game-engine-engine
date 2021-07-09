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
	if (material.color.rgb != vec3(0.0) && fragColor.rgb == vec3(0.0))	// use material.color if no texture is found
		fragColor = material.color;
	else if (fragColor.a < 0.5)
		discard;
	brightColor = fragColor;
	
	//fragColor = vec4(texCoord1, 0.0, 1.0);
		
	//fragColor.rgb *= fragColor.rgb + 1.0;
	//if (abs(textureSize(material.albedo1, 0).x - 546) < 2.0)
	{
		//fragColor = vec4(vec3(abs(textureSize(material.albedo1, 0).x)), 1.0);
		//fragColor.rgb /= fragColor.rgb + vec3(1.0);
		}
}
