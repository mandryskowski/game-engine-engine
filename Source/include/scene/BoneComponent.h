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
	Mat4f BoneOffset;
	Mat4f FinalMatrix;

public:
	BoneComponent(Actor&, Component* parentComp, std::string name, Transform t = Transform(), unsigned int boneID = 0);
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

	template <typename Archive> void Save(Archive& archive) const
	{
		archive(BoneID, BoneOffset, FinalMatrix, cereal::base_class<Component>(this));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		archive(BoneID, BoneOffset, FinalMatrix, cereal::base_class<Component>(this));
	}

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
	const Component* GlobalInverseTransformCompPtr;
	SkeletonBatch* BatchPtr;

	unsigned int BoneIDOffset;

public:
	SkeletonInfo();
	unsigned int GetBoneCount();
	SkeletonBatch* GetBatchPtr();
	unsigned int GetBoneIDOffset();
	int GetInfoID();
	void SetGlobalInverseTransformCompPtr(const Component* comp);
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
	template <typename Archive> void Save(Archive& archive) const
	{
		std::vector<std::string> bonesActorsNames, boneNames;	//TODO: add scene name string here and in Load(Archive&)

		for (auto it : Bones)
		{
			bonesActorsNames.push_back(it->GetActor().GetName());
			boneNames.push_back(it->GetName());
		}

		archive(cereal::make_nvp("BonesActorsNames", bonesActorsNames), 
				cereal::make_nvp("BoneNames", boneNames),
				cereal::make_nvp("GlobalInverseTransformCompPtrActorName", (GlobalInverseTransformCompPtr) ? (GlobalInverseTransformCompPtr->GetActor().GetName()) : (std::string())),
				cereal::make_nvp("GlobalInverseTransformCompPtrName", (GlobalInverseTransformCompPtr) ? (GlobalInverseTransformCompPtr->GetName()) : (std::string())),
				cereal::make_nvp("BatchID", BatchPtr->GetBatchID()),
				CEREAL_NVP(BoneIDOffset));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		std::vector<std::string> bonesActorsNames, boneNames;
		std::string globalInverseTCompActorName = (GlobalInverseTransformCompPtr) ? (GlobalInverseTransformCompPtr->GetActor().GetName()) : (std::string());
		std::string globalInverseTCompName = (GlobalInverseTransformCompPtr) ? (GlobalInverseTransformCompPtr->GetName()) : (std::string());
		int batchID;

		archive(cereal::make_nvp("BonesActorsNames", bonesActorsNames),
				cereal::make_nvp("BoneNames", boneNames),
				cereal::make_nvp("GlobalInverseTransformCompPtrActorName", globalInverseTCompActorName),
				cereal::make_nvp("GlobalInverseTransformCompPtrName", globalInverseTCompName),
				cereal::make_nvp("BatchID", batchID),
				CEREAL_NVP(BoneIDOffset));

		if (bonesActorsNames.size() != boneNames.size())
		{
			std::cout << "ERROR! BonesActorsNames' size different than boneNames'\n";
			return;
		}

		std::function<void()> loadBonesFunc = [this, bonesActorsNames, boneNames, globalInverseTCompActorName, globalInverseTCompName, batchID]() {
			BatchPtr = GameManager::DefaultScene->GetRenderData()->GetBatch(batchID);
			for (int i = 0; i < static_cast<int>(bonesActorsNames.size()); i++)
				if (BoneComponent* found = GameManager::DefaultScene->FindActor(bonesActorsNames[i])->GetRoot()->GetComponent<BoneComponent>(boneNames[i]))
				{
					Bones.push_back(found);
					found->SetInfoPtr(this);
				}
				else
					std::cout << "ERROR: Could not find bone " << boneNames[i] << " in actor " << bonesActorsNames[i] << ".\n";

			GlobalInverseTransformCompPtr = GameManager::DefaultScene->FindActor(globalInverseTCompActorName)->GetRoot()->GetComponent<Component>(globalInverseTCompName);

			SortBones();
			BatchPtr->RecalculateBoneCount();
		};

		GameManager::DefaultScene->AddPostLoadLambda(loadBonesFunc);
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
	int GetInfoID(SkeletonInfo&);
	SkeletonInfo* GetInfo(int ID);
	int GetBatchID();
	void RecalculateBoneCount();
	bool AddSkeleton(std::shared_ptr<SkeletonInfo>);
	void BindToUBO();

	template <typename Archive> void Serialize(Archive& archive)
	{
		archive(CEREAL_NVP(Skeletons), CEREAL_NVP(BoneCount));
	}
};

aiBone* FindAiBoneFromNode(const aiScene*, const aiNode*);
