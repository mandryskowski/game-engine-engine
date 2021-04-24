#include <scene/BoneComponent.h>
#include <assimp/scene.h>
#include <scene/HierarchyTemplate.h>
#include <rendering/Material.h>

BoneComponent::BoneComponent(Actor& actor, Component* parentComp, std::string name, Transform transform, unsigned int boneID):
	Component(actor, parentComp, name, transform),
	BoneID(boneID),
	BoneOffset(glm::mat4(1.0f)),
	FinalMatrix(glm::mat4(1.0f)),
	InfoPtr(nullptr)
{

}

BoneComponent::BoneComponent(BoneComponent&& bone):
	Component(std::move(bone)),
	BoneID(bone.BoneID),
	BoneOffset(bone.BoneOffset),
	FinalMatrix(bone.FinalMatrix),
	InfoPtr(bone.InfoPtr)
{
}

BoneComponent& BoneComponent::operator=(const BoneComponent& compT)
{
	Component::operator=(compT);

	BoneOffset = compT.BoneOffset;

	return *this;
}

void BoneComponent::Update(float deltaTime)
{
	glm::mat4 globalMat = ComponentTransform.GetWorldTransformMatrix();
	FinalMatrix = globalMat * BoneOffset;
	//ComponentTransform.Print(Name);

	ComponentTransform.Update(deltaTime);
}

unsigned int BoneComponent::GetID() const
{
	return BoneID;
}

const glm::mat4& BoneComponent::GetFinalMatrix()
{
	return FinalMatrix;
}

MaterialInstance BoneComponent::GetDebugMatInst(EditorIconState state)
{
	LoadDebugRenderMaterial("GEE_Mat_Default_Debug_BoneComponent", "EditorAssets/bonecomponent_icon.png");
	return Component::GetDebugMatInst(state);
}

void BoneComponent::SetBoneOffset(const glm::mat4& boneOffset)
{
	BoneOffset = boneOffset;
}

void BoneComponent::SetID(unsigned int id)
{
	BoneID = id;
}

void BoneComponent::SetInfoPtr(SkeletonInfo* infoPtr)
{
	InfoPtr = infoPtr;
}

BoneComponent::~BoneComponent()
{
	if (InfoPtr)
		InfoPtr->EraseBone(*this);
}

aiBone* FindAiBoneFromNode(const aiScene* scene, const aiNode* node)
{
	for (int i = 0; i < static_cast<int>(scene->mNumMeshes); i++)
		for (int j = 0; j < static_cast<int>(scene->mMeshes[i]->mNumBones); j++)
			if (node->mName == scene->mMeshes[i]->mBones[j]->mName)
				return scene->mMeshes[i]->mBones[j];

	return nullptr;
}


unsigned int BoneMapping::GetBoneID(std::string name)
{
	auto it = Mapping.find(name);
	if (it == Mapping.end())
	{
		unsigned int index = Mapping.size();
		Mapping[name] = index;
		return index;
	}

	return it->second;
}

unsigned int BoneMapping::GetBoneID(std::string name) const
{
	auto it = Mapping.find(name);
	if (it == Mapping.end())
		return 0;

	return it->second;
}

SkeletonInfo::SkeletonInfo():
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


void SkeletonInfo::SetGlobalInverseTransformCompPtr(const Component* comp)
{
	GlobalInverseTransformCompPtr = comp;
}

void SkeletonInfo::SetBatchData(SkeletonBatch* batch, unsigned int offset)
{
	BatchPtr = batch;
	BoneIDOffset = offset;
}

void SkeletonInfo::FillMatricesVec(std::vector<glm::mat4>& boneMats)
{
	//std::cout << "Filling " << Bones.size() << " bones\n";
	if (Bones.size() == 0)
		return;

	glm::mat4 globalInverseMat = glm::inverse(GlobalInverseTransformCompPtr->GetTransform().GetWorldTransformMatrix());

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

SkeletonBatch::SkeletonBatch():
	BoneCount(0)
{
	std::array<glm::mat4, 1024> identities;
	identities.fill(glm::mat4(1.0f));
	BoneUBO.Generate(10, sizeof(glm::mat4) * 1024, &identities[0][0][0], GL_STREAM_DRAW);
}

unsigned int SkeletonBatch::GetRemainingCapacity()
{
	return (MaxUBOSize / sizeof(glm::mat4)) - BoneCount;
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

void SkeletonBatch::RecalculateBoneCount()
{
	BoneCount = 0;
	for (int i = 0; i < static_cast<int>(Skeletons.size()); i++)
	{
		Skeletons[i]->SetBatchData(this, BoneCount);
		BoneCount += Skeletons[i]->GetBoneCount();
	}
}

bool SkeletonBatch::AddSkeleton(std::shared_ptr<SkeletonInfo> info)
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
	std::vector<glm::mat4> boneMats;
	boneMats.resize(BoneCount, glm::mat4(1.0f));

	for (int i = 0; i < static_cast<int>(Skeletons.size()); i++)
	{
		Skeletons[i]->FillMatricesVec(boneMats);
	}

	BoneUBO.SubData(BoneCount * sizeof(glm::mat4), &boneMats[0][0][0], 0);
}
