#version 400 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;

//out
out VS_OUT
{
	vec3 position;
	vec2 texCoord;
	
	mat3 TBN;
}	vs_out;

//uniforms
uniform mat4 model;
uniform mat4 MVP;
uniform mat3 normalMat;
layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};

void main()
{
	vs_out.position = vec3(model * vec4(vPosition, 1.0));
	vs_out.texCoord = vTexCoord;
	
	vec3 T = normalize(vTangent);
	vec3 B;
	vec3 N = normalize(vNormal);
	if (true)//B == vec3(0.0))
		B = normalize(cross(N, T));
	else
		B = normalize(vBitangent);
	
	T = normalize(normalMat * T);
	B = normalize(normalMat * B);
	N = normalize(normalMat * N);
	
	vs_out.TBN = mat3(T, B, N);
	
	gl_Position = MVP * vec4(vPosition, 1.0);
}