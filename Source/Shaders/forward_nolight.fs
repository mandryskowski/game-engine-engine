#version 400 core

struct Material
{
	sampler2D diffuse1;
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
	fragColor = mix(texture(material.diffuse1, texCoord1), texture(material.diffuse1, texCoord2), blend);
	brightColor = fragColor;
	
	//fragColor = vec4(texCoord1, 0.0, 1.0);
	
	if (fragColor.a < 0.5)
		discard;
}