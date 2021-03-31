#include <scene/BoneComponent.h>
#include <assimp/scene.h>
#include <rendering/MeshSystem.h>
#include <rendering/Material.h>

BoneComponent::BoneComponent(GameScene& scene, std::string name, Transform transform, unsigned int boneID):
	Component(scene, name, transform),
	BoneID(boneID),
	BoneOffset(glm::mat4(1.0f))
{

}

BoneComponent::BoneComponent(BoneComponent&& bone):
	Component(std::move(bone)),
	BoneID(bone.BoneID),
	BoneOffset(bone.BoneOffset),
	FinalMatrix(bone.FinalMatrix)
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

void BoneComponent::GenerateFromNode(const MeshSystem::TemplateNode& node, Material* overrideMaterial)
{
	Component::GenerateFromNode(node);
	const MeshSystem::BoneNode* boneNodeCast = dynamic_cast<const MeshSystem::BoneNode*>(&node);
	BoneOffset = boneNodeCast->GetBoneOffset();
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
	GlobalInverseTransformPtr(nullptr),
	IDOffset(0),
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

unsigned int SkeletonInfo::GetIDOffset()
{
	return IDOffset;
}


void SkeletonInfo::SetGlobalInverseTransformPtr(const Transform* transformPtr)
{
	GlobalInverseTransformPtr = transformPtr;
}

void SkeletonInfo::SetBatchData(SkeletonBatch* batch, unsigned int offset)
{
	BatchPtr = batch;
	IDOffset = offset;
}

void SkeletonInfo::FillMatricesVec(std::vector<glm::mat4>& boneMats)
{
	//std::cout << "Filling " << Bones.size() << " bones\n";
	if (Bones.size() == 0)
		return;

	glm::mat4 globalInverseMat = glm::inverse(GlobalInverseTransformPtr->GetWorldTransformMatrix());

	for (int i = 0; i < static_cast<int>(Bones.size()); i++)
	{
		boneMats[Bones[i]->GetID() + IDOffset] = globalInverseMat * Bones[i]->GetTransform().GetWorldTransformMatrix() * Bones[i]->BoneOffset;
	}
}

void SkeletonInfo::AddBone(BoneComponent& bone)
{
	Bones.push_back(&bone);
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
	BoneUBO.Generate(2, sizeof(glm::mat4) * 1024, &identities[0][0][0], GL_STREAM_DRAW);
}

unsigned int SkeletonBatch::GetRemainingCapacity()
{
	return (MaxUBOSize / sizeof(glm::mat4)) - BoneCount;
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
