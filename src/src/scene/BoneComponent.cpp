#include <scene/BoneComponent.h>
#include <assimp/scene.h>
#include <rendering/Material.h>
#include <animation/SkeletonInfo.h>

namespace GEE
{
	BoneComponent::BoneComponent(Actor& actor, Component* parentComp, std::string name, Transform transform, unsigned int boneID) :
		Component(actor, parentComp, name, transform),
		BoneID(boneID),
		BoneOffset(Mat4f(1.0f)),
		FinalMatrix(Mat4f(1.0f)),
		InfoPtr(nullptr)
	{

	}

	BoneComponent::BoneComponent(BoneComponent&& bone) :
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
		Mat4f globalMat = ComponentTransform.GetWorldTransformMatrix();
		FinalMatrix = globalMat * BoneOffset;
		//ComponentTransform.Print(Name);

		ComponentTransform.Update(deltaTime);
	}

	unsigned int BoneComponent::GetID() const
	{
		return BoneID;
	}

	const Mat4f& BoneComponent::GetFinalMatrix()
	{
		return FinalMatrix;
	}

	MaterialInstance BoneComponent::LoadDebugMatInst(EditorButtonState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_BoneComponent", "Assets/Editor/bonecomponent_icon.png");
		return Component::LoadDebugMatInst(state);
	}

	void BoneComponent::SetBoneOffset(const Mat4f& boneOffset)
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


}