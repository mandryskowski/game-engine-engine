#pragma once
#include <game/GameManager.h>
#include <utility/CerealNames.h>
#include <cereal/types/memory.hpp>
#include <cereal/access.hpp>


namespace GEE
{
	class SkeletonBatch;
	class BoneComponent;

	class SkeletonInfo
	{
	public:
		SkeletonInfo();
		unsigned int GetBoneCount() const;
		SkeletonBatch* GetBatchPtr() const;
		unsigned int GetBoneIDOffset() const;
		int GetInfoID();
		void AddModelCompRef(ModelComponent&);
		void EraseModelCompRef(ModelComponent&);
		void SetGlobalInverseTransformCompPtr(const Component* comp);
		void SetBatchData(SkeletonBatch* batch, unsigned int idOffset);
		bool VerifyGlobalInverseCompPtrLife();	//Call every frame
		void FillMatricesVec(std::vector<Mat4f>&);
		void AddBone(BoneComponent&);
		void EraseBone(BoneComponent&);
		void SortBones();	//Sorts bones by id. May improve performance
		~SkeletonInfo();
		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);

	private:
		std::vector<BoneComponent*> Bones;
		std::vector<ModelComponent*> ModelsRef;
		const Component* GlobalInverseTransformCompPtr;
		SkeletonBatch* BatchPtr;

		unsigned int BoneIDOffset;

	};

	class SkeletonBatch
	{
		std::vector<SharedPtr<SkeletonInfo>> Skeletons;
		unsigned int BoneCount;

		UniformBuffer BoneUBOs[2];
		unsigned int CurrentBoneUBOIndex;

		friend class RenderEngine;
		friend class GameRenderer;
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
}