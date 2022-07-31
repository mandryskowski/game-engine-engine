layout (location = 0) in vec3 vPosition;

//out
out vec3 nearPos;
out vec3 farPos;
out mat4 fragVP;

//uniform
uniform mat4 VP;

void main()
{
	mat4 inverseVP = inverse(VP);
	vec4 nearProj = inverseVP * vec4(vPosition.xy, 0.0, 1.0);
	vec4 farProj = inverseVP  * vec4(vPosition.xy, 1.0, 1.0);
	
	nearPos = nearProj.xyz / nearProj.w;
	farPos = farProj.xyz / farProj.w;
	
	fragVP = VP;
	
	gl_Position = vec4(vPosition, 1.0);
}