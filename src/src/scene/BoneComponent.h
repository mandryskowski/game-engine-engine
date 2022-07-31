#pragma once

#include <scene/Component.h>
#include <scene/hierarchy/HierarchyNodeInstantiation.h>
#include <animation/SkeletonInfo.h>
#include <assimp/types.h>
#include <map>

struct aiScene;
struct aiNode;
struct aiBone;

namespace GEE
{
	constexpr int MaxUBOSize = 65536;


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
		friend class Hierarchy::Node<BoneComponent>;
		BoneComponent& operator=(const BoneComponent&);
	public:
		BoneComponent& operator=(BoneComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy

		virtual void Update(float deltaTime) override;
		unsigned int GetID() const;
		const Mat4f& GetFinalMatrix();

		virtual	MaterialInstance GetDebugMatInst(ButtonMaterialType) override;
		 
		void SetBoneOffset(const Mat4f&);
		void SetID(unsigned int id);
		void SetInfoPtr(SkeletonInfo*);

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(BoneID), CEREAL_NVP(BoneOffset), CEREAL_NVP(FinalMatrix), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			archive(CEREAL_NVP(BoneID), CEREAL_NVP(BoneOffset), CEREAL_NVP(FinalMatrix), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
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

	aiBone* FindAiBoneFromNode(const aiScene*, const aiNode*);

	template <> inline void Hierarchy::Instantiation::Impl<BoneComponent>::InstantiateToComp(SharedPtr<Instantiation::Data> data, const NodeBase& instantiatedNode, BoneComponent& targetComp)
	{
		Impl<Component>::InstantiateToComp(data, instantiatedNode, targetComp);

		GEE_CORE_ASSERT(data->GetTree().ContainsBones());

		if (data->GetSkeletonInfo())
			data->GetSkeletonInfo()->AddBone(targetComp);
	}
}
GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::BoneComponent)