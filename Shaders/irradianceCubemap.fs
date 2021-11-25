#define M_PI 3.14159

//in
in vec3 localPosition;

//out
out vec4 fragColor;

//uniform
uniform float cubemapNr;
uniform samplerCube cubemap;

void main()
{
	vec3 n = normalize(localPosition);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = cross(up, n);
	up = cross(n, right);
	
	float stepSize = M_PI * 0.007;
	float sampleCount = 0.0;
	vec3 irradiance = vec3(0.0);
	for (float phi = 0.0; phi < 2.0 * M_PI; phi += stepSize)
	{
		for (float theta = 0.0; theta < M_PI * 0.5; theta += stepSize)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			vec3 worldSample = tangentSample.x * right + tangentSample.y * up + tangentSample.z * n;
			
			irradiance += texture(cubemap, vec3(worldSample)).rgb * sin(theta) * cos(theta);
			sampleCount++;
		}
	}
	
	fragColor = vec4(M_PI * irradiance / sampleCount, 1.0);
}