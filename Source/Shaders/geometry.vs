layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;

//out
out VS_OUT
{
	vec3 worldPosition;
	#ifdef CALC_VELOCITY_BUFFER
	smooth vec4 prevMVPPosition;
	#endif
	
	vec2 texCoord;
	
	mat3 TBN;
}	vs_out;

//uniform
uniform mat4 model;
uniform mat4 MVP;
#ifdef CALC_VELOCITY_BUFFER
uniform mat4 prevMVP;
#endif
uniform mat3 normalMat;

void main()
{
	vs_out.worldPosition = vec3(model * vec4(vPosition, 1.0));
	vs_out.texCoord = vTexCoord;
	
	vec3 T = normalize(normalMat * vTangent);
	vec3 B = normalize(normalMat * vBitangent);
	vec3 N = normalize(normalMat * vNormal);
	if (vBitangent == vec3(0.0))
		B = normalize(normalMat * cross(T, N));
	
	vs_out.TBN = mat3(T, B, N);
	
	#ifdef CALC_VELOCITY_BUFFER
	vs_out.prevMVPPosition = prevMVP * vec4(vPosition, 1.0);
	#endif
	
	gl_Position = MVP * vec4(vPosition, 1.0);
}