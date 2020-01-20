#version 330 core

//in
in vec2 texCoord;

//out
out vec4 fragColor;

//uniform
uniform bool horizontal;
uniform sampler2D tex;

void main()
{
	float weights[] = float[](0.22702, 0.31621, 0.07026);
	float offsets[] = float[](0.0, 1.38461, 3.23071);
	vec2 unitOffset = 1.0 / textureSize(tex, 0);
	vec3 color = texture(tex, texCoord).rgb * weights[0];

	if (horizontal)
	{
		for (int i = 1; i < 3; i++)
		{
			color += texture(tex, texCoord + vec2(offsets[i] * unitOffset.x, 0.0)).rgb * weights[i];
			color += texture(tex, texCoord - vec2(offsets[i] * unitOffset.x, 0.0)).rgb * weights[i];
		}
	}
	else
	{
		for (int i = 1; i < 3; i++)
		{
			color += texture(tex, texCoord + vec2(0.0, offsets[i] * unitOffset.y)).rgb * weights[i];
			color += texture(tex, texCoord - vec2(0.0, offsets[i] * unitOffset.y)).rgb * weights[i];
		}
	}
	
	fragColor = vec4(color, 1.0);
}