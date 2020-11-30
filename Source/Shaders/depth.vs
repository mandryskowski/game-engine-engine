#define BONE_MATS_BATCH_SIZE 1024
layout (location = 0) in vec3 vPosition;
layout (location = 5) in ivec4 vBoneIDs;
layout (location = 6) in vec4 vBoneWeights;

//uniform
uniform mat4 MVP;

uniform int boneIDOffset;
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
	
	gl_Position = MVP * vec4(bonePosition.xyz, 1.0);
}