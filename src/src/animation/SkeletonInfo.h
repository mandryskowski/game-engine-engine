#pragma once
#include <game/GameScene.h>
#include <game/GameManager.h>
#include <cereal/types/memory.hpp>
#include <cereal/access.hpp>
#include <scene/BoneComponent.h>


namespace GEE
{
	class SkeletonBatch;
	class BoneComponent;

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
		bool VerifyGlobalInverseCompPtrLife();	//Call every frame
		void FillMatricesVec(std::vector<Mat4f>&);
		void AddBone(BoneComponent&);
		void EraseBone(BoneComponent&);
		void SortBones();	//Sorts bones by id. May improve performance
		/*void DRAWBATCH() const
		{
			return;
			std::cout << "----\n";
			for (int i = 0; i < static_cast<int>(Bones.size()); i++)
			{
				std::cout << Bones[i]->GetName() << "!!!: " << Bones[i]->GetID() << ".\n";
			}
			std::cout << "----\n";
		}*/
		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);
	};

	class SkeletonBatch
	{
		std::vector<SharedPtr<SkeletonInfo>> Skeletons;
		unsigned int BoneCount;

		UniformBuffer BoneUBOs[2];
		unsigned int CurrentBoneUBOIndex;

		friend class RenderEngine;
		void SwapBoneUBOs();
	public:
		SkeletonBatch();
		unsigned int GetRemainingCapacity();
		int GetInfoID(SkeletonInfo&);
		SkeletonInfo* GetInfo(int ID);
		int GetBatchID();
		UniformBuffer GetBoneUBO();
		void RecalculateBoneCount();
		bool AddSkeleton(SharedPtr<SkeletonInfo>);
		void BindToUBO();
		void VerifySkeletonsLives();	//Call every frame

		template <typename Archive> void Serialize(Archive& archive)
		{
			archive(CEREAL_NVP(Skeletons), CEREAL_NVP(BoneCount));
		}
	};

	/*void DRAWBATCH() const
	{
	return;
	std::cout << "----\n";
	for (int i = 0; i < static_cast<int>(Bones.size()); i++)
	{
	std::cout << Bones[i]->GetName() << "!!!: " << Bones[i]->GetID() << ".\n";
	}
	std::cout << "----\n";
	}*/

	template<typename Archive>
	inline void SkeletonInfo::Save(Archive& archive) const
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
	template<typename Archive>
	inline void SkeletonInfo::Load(Archive& archive)
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
}