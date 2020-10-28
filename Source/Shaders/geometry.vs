#define BONE_MATS_BATCH_SIZE 1024
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in vec3 vBitangent;
layout (location = 5) in ivec4 vBoneIDs;
layout (location = 6) in vec4 vBoneWeights;

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
uniform int boneIDOffset;
uniform mat4 model;
uniform mat4 MVP;
#ifdef CALC_VELOCITY_BUFFER
uniform mat4 prevMVP;
#endif
uniform mat3 normalMat;
layout (std140) uniform BoneMatrices
{
	mat4 boneMatrices[BONE_MATS_BATCH_SIZE];
};

void main()
{
	mat4 boneMatrix = boneMatrices[vBoneIDs[0] + boneIDOffset] * vBoneWeights[0];
	for (int i = 1; i < 4; i++)
		boneMatrix += boneMatrices[vBoneIDs[i] + boneIDOffset] * vBoneWeights[i];
		
	if (vBoneIDs.x == 0 && vBoneIDs.y == 0)
		boneMatrix = mat4(1.0);
		
	vec4 bonePosition = boneMatrix * vec4(vPosition, 1.0);
		
	vs_out.worldPosition = vec3(model * bonePosition);
	vs_out.texCoord = vTexCoord;
	
	mat3 normalBoneMat = normalMat * mat3(transpose(inverse(boneMatrix)));
	
	vec3 T = normalize(normalBoneMat * vTangent);
	vec3 B = normalize(normalBoneMat * vBitangent);
	vec3 N = normalize(normalBoneMat * vNormal);
		B = normalize(normalBoneMat * cross(N, T));
	
	vs_out.TBN = mat3(T, B, N);
	
	#ifdef CALC_VELOCITY_BUFFER
	vs_out.prevMVPPosition = prevMVP * vec4(vPosition, 1.0);
	#endif

	gl_Position = MVP * bonePosition;
}