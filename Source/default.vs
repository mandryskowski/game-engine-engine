#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

//out
out VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	
	mat3 TBN;
}	vs_out;

//uniforms
uniform vec3 scale;
uniform mat4 model;
uniform mat4 modelView;
uniform mat3 normalMat;
layout (std140) uniform Matrices
{
	mat4 view;
	mat4 projection;
};

void main()
{
	vs_out.position = vec3(model * vec4(vPosition, 1.0));
	vs_out.normal = normalize(normalMat * vNormal);
	
	vec3 T = normalize(vTangent);
	vec3 B;
	vec3 N = normalize(vNormal);
	B = normalize(cross(T, N));
	
	//scale the texture coords along with the model (so the textures repeat instead of stretching
	vec2 tangentScale = vec2(transpose(mat3(T, B, N)) * scale);
	tangentScale = abs(tangentScale);
	vs_out.texCoord = vTexCoord * tangentScale.xy;
	
	T = normalize(normalMat * T);
	B = normalize(normalMat * B);
	N = normalize(normalMat * N);
	
	vs_out.TBN = mat3(T, B, N);
	
	gl_Position = projection * modelView * vec4(vPosition, 1.0);
}