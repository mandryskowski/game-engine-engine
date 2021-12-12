#include <animation/SkeletonInfo.h>
#include <game/GameScene.h>
#include <scene/BoneComponent.h>
#include <scene/ModelComponent.h>

namespace GEE
{
	SkeletonInfo::SkeletonInfo() :
		GlobalInverseTransformCompPtr(nullptr),
		BoneIDOffset(0),
		BatchPtr(nullptr)
	{
	}

	unsigned int SkeletonInfo::GetBoneCount()
	{
		return Bones.size();
	}

	SkeletonBatch* SkeletonInfo::GetBatchPtr()
	{
		return BatchPtr;
	}

	unsigned int SkeletonInfo::GetBoneIDOffset()
	{
		return BoneIDOffset;
	}

	int SkeletonInfo::GetInfoID()
	{
		return (BatchPtr) ? (BatchPtr->GetInfoID(*this)) : (-1);
	}

	void SkeletonInfo::AddModelCompRef(ModelComponent& model)
	{
		ModelsRef.push_back(&model);
		std::cout << "(%) " << this << ", " << ModelsRef.size() << " models.\n";
	}

	void SkeletonInfo::EraseModelCompRef(ModelComponent& model)
	{
		ModelsRef.erase(std::remove_if(ModelsRef.begin(), ModelsRef.end(), [&model](ModelComponent* modelVec) { return modelVec == &model; }), ModelsRef.end());
	}

	void SkeletonInfo::SetGlobalInverseTransformCompPtr(const Component* comp)
	{
		GlobalInverseTransformCompPtr = comp;
	}

	void SkeletonInfo::SetBatchData(SkeletonBatch* batch, unsigned int offset)
	{
		BatchPtr = batch;
		BoneIDOffset = offset;
	}

	bool SkeletonInfo::VerifyGlobalInverseCompPtrLife()
	{
		if (!GlobalInverseTransformCompPtr || GlobalInverseTransformCompPtr->IsBeingKilled())
			return false;

		return true;
	}

	void SkeletonInfo::FillMatricesVec(std::vector<Mat4f>& boneMats)
	{
		//std::cout << "Filling " << Bones.size() << " bones\n";
		if (Bones.size() == 0)
			return;

		Mat4f globalInverseMat = glm::inverse(GlobalInverseTransformCompPtr->GetTransform().GetWorldTransformMatrix());

		//std::cout << "nr bones: " << Bones.size() << '\n';

		for (int i = 0; i < static_cast<int>(Bones.size()); i++)
		{
			boneMats[Bones[i]->GetID() + BoneIDOffset] = globalInverseMat * Bones[i]->GetTransform().GetWorldTransformMatrix() * Bones[i]->BoneOffset;
			//std::cout << Bones[i]->GetName() << " " << Bones[i]->GetID() + IDOffset << '\n';
		}
	}

	void SkeletonInfo::AddBone(BoneComponent& bone)
	{
		Bones.push_back(&bone);
		bone.SetInfoPtr(this);
	}

	void SkeletonInfo::EraseBone(BoneComponent& bone)
	{
		Bones.erase(std::remove_if(Bones.begin(), Bones.end(), [&bone](BoneComponent* boneVec) { /*if (boneVec == &bone) std::cout << "erased bone " << bone.GetName() << '\n';*/ return boneVec == &bone; }), Bones.end());
	}

	void SkeletonInfo::SortBones()
	{
		std::sort(Bones.begin(), Bones.end(), [](BoneComponent* bone1, BoneComponent* bone2) { return bone2->GetID() > bone1->GetID(); });
	}

	void SkeletonBatch::SwapBoneUBOs()
	{
		BoneUBOs[CurrentBoneUBOIndex].BindToSlot(11, false);
		CurrentBoneUBOIndex = (CurrentBoneUBOIndex == 0) ? (1) : (0);

		if (!BoneUBOs[1].HasBeenGenerated())
		{
			std::array<Mat4f, 1024> identities;
			identities.fill(Mat4f(1.0f));
			BoneUBOs[1].Generate(10, sizeof(Mat4f) * 1024, &identities[0][0][0], GL_STREAM_DRAW);
		}
		BoneUBOs[CurrentBoneUBOIndex].BindToSlot(10, false);
	}

	template<typename Archive>
	void SkeletonInfo::Save(Archive& archive) const
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
	void SkeletonInfo::Load(Archive& archive)
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

	template void SkeletonInfo::Save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&) const;
	template void SkeletonInfo::Load<cereal::JSONInputArchive>(cereal::JSONInputArchive&);

	SkeletonInfo::~SkeletonInfo()
	{
		std::cout << "SKELETON INFO BEING DESTROYED. " << ModelsRef.size() << " MODELS\n";
		for (auto it : ModelsRef)
			it->SignalSkeletonInfoDeath();
	}

	SkeletonBatch::SkeletonBatch() :
		BoneCount(0),
		CurrentBoneUBOIndex(0)
	{
		std::array<Mat4f, 1024> identities;
		identities.fill(Mat4f(1.0f));
		BoneUBOs[0].Generate(10, sizeof(Mat4f) * 1024, &identities[0][0][0], GL_STREAM_DRAW);
	}

	unsigned int SkeletonBatch::GetRemainingCapacity()
	{
		return (MaxUBOSize / sizeof(Mat4f)) - BoneCount;
	}

	int SkeletonBatch::GetInfoID(SkeletonInfo& info)
	{
		for (int i = 0; i < static_cast<int>(Skeletons.size()); i++)
			if (Skeletons[i].get() == &info)
				return static_cast<unsigned int>(i);

		return -1;	//info not found; return invalid ID
	}

	SkeletonInfo* SkeletonBatch::GetInfo(int ID)
	{
		if (ID < 0 || ID > Skeletons.size() - 1)
			return nullptr;

		return Skeletons[ID].get();
	}

	int SkeletonBatch::GetBatchID()
	{
		return GameManager::DefaultScene->GetRenderData()->GetBatchID(*this);
	}

	UniformBuffer SkeletonBatch::GetBoneUBO()
	{
		return BoneUBOs[CurrentBoneUBOIndex];
	}

	void SkeletonBatch::RecalculateBoneCount()
	{
		BoneCount = 0;
		for (int i = 0; i < static_cast<int>(Skeletons.size()); i++)
		{
			Skeletons[i]->SetBatchData(this, BoneCount);
			BoneCount += Skeletons[i]->GetBoneCount();
		}
	}

	bool SkeletonBatch::AddSkeleton(SharedPtr<SkeletonInfo> info)
	{
		RecalculateBoneCount();
		if (GetRemainingCapacity() < info->GetBoneCount())
		{
			std::cout << "INFO: Too many bones in skeleton batch!\n";
			return false;
		}

		Skeletons.push_back(info);
		Skeletons.back()->SetBatchData(this, BoneCount);
		BoneCount += info->GetBoneCount();

		return true;
	}

	void SkeletonBatch::BindToUBO()
	{
		std::vector<Mat4f> boneMats;
		boneMats.resize(BoneCount, Mat4f(1.0f));

		for (int i = 0; i < static_cast<int>(Skeletons.size()); i++)
		{
			Skeletons[i]->FillMatricesVec(boneMats);
		}

		if (!boneMats.empty())
			GetBoneUBO().SubData(BoneCount * sizeof(Mat4f), &boneMats[0][0][0], 0);
	}

	void SkeletonBatch::VerifySkeletonsLives()
	{
		Skeletons.erase(std::remove_if(Skeletons.begin(), Skeletons.end(), [](SharedPtr<SkeletonInfo>& skeleton) { return !skeleton->VerifyGlobalInverseCompPtrLife(); /* Remove if global inverse comp is not alive. */ }), Skeletons.end());
	}

}