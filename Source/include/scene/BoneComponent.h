#pragma once

#include <scene/Component.h>
#include <animation/Animation.h>
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
	SkeletonInfo* InfoPtr;
public:
	glm::mat4 BoneOffset;
	glm::mat4 FinalMatrix;

public:
	BoneComponent(GameScene&, std::string name, Transform t = Transform(), unsigned int boneID = 0);
	BoneComponent(BoneComponent&&);

protected:
	friend class HierarchyTemplate::HierarchyNode<BoneComponent>;
	BoneComponent& operator=(const BoneComponent&);
public:
	BoneComponent& operator=(BoneComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy

	virtual void Update(float deltaTime) override;
	unsigned int GetID() const;
	const glm::mat4& GetFinalMatrix();

	virtual MaterialInstance GetDebugMatInst(EditorIconState) override;

	void SetBoneOffset(const glm::mat4&);
	void SetID(unsigned int id);
	void SetInfoPtr(SkeletonInfo*);

	virtual ~BoneComponent();
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
	void EraseBone(BoneComponent&);
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

public:
	SkeletonBatch();
	unsigned int GetRemainingCapacity();
	void RecalculateBoneCount();
	bool AddSkeleton(std::shared_ptr<SkeletonInfo>);
	void BindToUBO();
};

aiBone* FindAiBoneFromNode(const aiScene*, const aiNode*);
