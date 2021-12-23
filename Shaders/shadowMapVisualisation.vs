#define BONE_MATS_BATCH_SIZE 1024
layout (location = 0) in vec3 vPosition;
layout (location = 5) in ivec4 vBoneIDs;
layout (location = 6) in vec4 vBoneWeights;

//out
out vec3 fragPosition;

//uniform
uniform int boneIDOffset;
uniform mat4 model;
uniform mat4 MVP;
layout (std140) uniform BoneMatrices
{
	mat4 boneMatrices[BONE_MATS_BATCH_SIZE];
};

void main()
{
	mat4 boneMatrix = boneMatrices[vBoneIDs[0] + boneIDOffset] * vBoneWeights[0];
	for (int i = 1; i < 4; i++)
		boneMatrix += boneMatrices[vBoneIDs[i] + boneIDOffset] * vBoneWeights[i];
		
	if (vBoneIDs.x == 0 && vBoneWeights.x == 0.0)	//If no bones are bound to this vertex, ignore any bone matrices. Note: we also check the first vBoneWeight, because a bone can have index 0 (its also the default value of vBoneIDs - ivec4(0)).
		boneMatrix = mat4(1.0);
		
	vec4 bonePosition = boneMatrix * vec4(vPosition, 1.0);
		
	fragPosition = vec3(model * bonePosition);
	gl_Position = MVP * bonePosition;
}