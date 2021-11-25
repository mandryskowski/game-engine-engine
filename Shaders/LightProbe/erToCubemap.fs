#define M_INV_ATAN vec2(0.1591, 0.3183)

//in
in vec3 localPosition;

//out
out vec4 fragColor;

//uniform
uniform sampler2D tex;

vec4 SampleEquirectangularTex(sampler2D erTex, vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= M_INV_ATAN;
	uv += 0.5;
	
	return texture(erTex, uv);
}

void main()
{
	fragColor = SampleEquirectangularTex(tex, normalize(localPosition));
}