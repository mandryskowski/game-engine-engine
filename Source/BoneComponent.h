#pragma once

#include "Component.h"
#include "Animation.h"
#include <assimp/types.h>
#include <map>

constexpr int MaxUBOSize = 65536;

struct aiScene;
struct aiNode;
struct aiBone;

class SkeletonBatch;


class BoneComponent : public Component
{
	unsigned int BoneID;
public:
	glm::mat4 BoneOffset;
	glm::mat4 FinalMatrix;

public:
	BoneComponent(GameManager*, std::string name, Transform t, unsigned int boneID);
	virtual void Update(float deltaTime) override;
	virtual void GenerateFromNode(const MeshSystem::TemplateNode* node, Material* overrideMaterial = nullptr) override;
	unsigned int GetID() const;
	const glm::mat4& GetFinalMatrix();
};

class BoneMapping
{
	std::map<std::string, unsigned int> Mapping;	/*Maps bone name and index. It's a template object used for loading.
												When you load a mesh by moving recursively through the assimp hierarchy (as we do here) a mesh may reference a bone that is not converted to our structure.
												So we assign a bone id to that not-created name and we use this mapping for assigning the bone ID of the actual BoneComponent.*/
public:
	unsigned int GetBoneID(std::string name);
	unsigned int GetBoneID(std::string name) const;
};

class SkeletonInfo
{
	std::vector<BoneComponent*> Bones;
	const Transform* GlobalInverseTransformPtr;
	SkeletonBatch* BatchPtr;
	unsigned int IDOffset;

public:
	SkeletonInfo();
	unsigned int GetBoneCount();
	SkeletonBatch* GetBatchPtr();
	unsigned int GetIDOffset();
	void SetGlobalInverseTransformPtr(const Transform* transformPtr);
	void SetBatchData(SkeletonBatch* batch, unsigned int idOffset);
	void FillMatricesVec(std::vector<glm::mat4>&);
	void AddBone(BoneComponent&);
	void SortBones();	//Sorts bones by id. May improve performance
	void DRAWBATCH() const
	{
		return;
		std::cout << "----\n";
		for (int i = 0; i < static_cast<int>(Bones.size()); i++)
		{
			std::cout << Bones[i]->GetName() << "!!!: " << Bones[i]->GetID() << ".\n";
		}
		std::cout << "----\n";
	}
};

class SkeletonBatch
{
	std::vector<std::shared_ptr<SkeletonInfo>> Skeletons;
	unsigned int BoneCount;
	UniformBuffer BoneUBO;

	void RecalculateBoneCount();
public:
	SkeletonBatch();
	unsigned int GetRemainingCapacity();
	bool AddSkeleton(std::shared_ptr<SkeletonInfo>);
	void BindToUBO();
};

aiBone* FindAiBoneFromNode(const aiScene*, const aiNode*);
