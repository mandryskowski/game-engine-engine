#define BONE_MATS_BATCH_SIZE 1024
layout (location = 0) in vec3 vPosition;
#ifdef EXTRUDE_VERTICES
layout (location = 1) in vec3 vNormal;
#endif
#ifdef OUTLINE_DISCARD_ALPHA
layout (location = 2) in vec2 vTexCoord;
#endif
layout (location = 5) in ivec4 vBoneIDs;
layout (location = 6) in vec4 vBoneWeights;

//out
#ifdef OUTLINE_DISCARD_ALPHA
out vec2 texCoord;
#endif

//uniform
uniform mat4 MVP;
uniform int boneIDOffset;
layout (std140) uniform BoneMatrices
{
	mat4 boneMatrices[BONE_MATS_BATCH_SIZE];
};

#ifdef EXTRUDE_VERTICES
//uniform float vertexExtrusionFactor;
#define vertexExtrusionFactor 0.004
#endif

void main()
{	
	mat4 boneMatrix = boneMatrices[vBoneIDs[0] + boneIDOffset] * vBoneWeights[0];
	for (int i = 1; i < 4; i++)
		boneMatrix += boneMatrices[vBoneIDs[i] + boneIDOffset] * vBoneWeights[i];
		
	if (vBoneIDs.x == 0 && vBoneWeights.x == 0.0)	//If no bones are bound to this vertex, ignore any bone matrices. Note: we also check the first vBoneWeight, because a bone can have index 0 (its also the default value of vBoneIDs - ivec4(0)).
		boneMatrix = mat4(1.0);
		
	vec4 bonePosition = boneMatrix * vec4(vPosition, 1.0);
	
	#ifdef EXTRUDE_VERTICES
	bonePosition = vec4(bonePosition.xyz +  vNormal * vertexExtrusionFactor, 1.0);
	#endif
	gl_Position = MVP * vec4(bonePosition.xyz, 1.0);
	
	#ifdef OUTLINE_DISCARD_ALPHA
	texCoord = vTexCoord;
	#endif
}