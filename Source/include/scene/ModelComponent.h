#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Mesh.h>
#include "physics/CollisionObject.h"
#include <UI/UIComponent.h>

namespace GEE
{
	namespace MeshSystem
	{
		class MeshNodeInstance;
		class MeshNode;
	}

	//Todo: Usun SkeletonInfo z ModelComponent
	//Dodaj to do osobnej klasy SkeletalModelComponent ktora dziedziczy z ModelComponent


	class ModelComponent : public RenderableComponent, public UIComponent
	{
	public:
		ModelComponent(Actor&, Component* parentComp, const std::string& name = "undefinedModel", const Transform & = Transform(), SkeletonInfo* info = nullptr, Material* overrideMat = nullptr);

		ModelComponent(const ModelComponent&) = delete;
		ModelComponent(ModelComponent&&);

	protected:
		friend class HierarchyTemplate::HierarchyNode<ModelComponent>;
		ModelComponent& operator=(const ModelComponent&);
	public:
		ModelComponent& operator=(ModelComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy

		void OverrideInstancesMaterial(Material* material);	//Overrides MeshInstances' materials with the one passed as the argument
		void OverrideInstancesMaterialInstances(std::shared_ptr<MaterialInstance> matInst);	//Overrides MeshInstances' materials with the one passed as the argument
		void SetLastFrameMVP(const glm::mat4& lastMVP) const;
		void SetSkeletonInfo(SkeletonInfo*);
		void SetRenderAsBillboard(bool billboard);

		void DRAWBATCH() const;

		int GetMeshInstanceCount() const;
		const MeshInstance& GetMeshInstance(int index) const;
		const glm::mat4& GetLastFrameMVP() const;
		SkeletonInfo* GetSkeletonInfo() const;

		MeshInstance* FindMeshInstance(const std::string& nodeName, const std::string& specificMeshName = std::string());
		void AddMeshInst(const MeshInstance&);

		virtual void Update(float deltaTime) override;

		virtual void Render(const RenderInfo&, Shader* shader) override;

		virtual void GetEditorDescription(EditorDescriptionBuilder);

		template <typename Archive> void Save(Archive& archive) const
		{
			archive(CEREAL_NVP(RenderAsBillboard), CEREAL_NVP(MeshInstances), cereal::make_nvp("SkelInfoBatchID", (SkelInfo) ? (SkelInfo->GetBatchPtr()->GetBatchID()) : (-1)), cereal::make_nvp("SkelInfoID", (SkelInfo) ? (SkelInfo->GetInfoID()) : (-1)), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
		}
		template <typename Archive> void Load(Archive& archive)	//Assumptions: Order of batches and their Skeletons is not changed during loading.
		{
			int skelInfoBatchID, skelInfoID;
			archive(CEREAL_NVP(RenderAsBillboard), CEREAL_NVP(MeshInstances), cereal::make_nvp("SkelInfoBatchID", skelInfoBatchID), cereal::make_nvp("SkelInfoID", skelInfoID), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
			if (skelInfoID >= 0)
			{
				SkelInfo = Scene.GetRenderData()->GetBatch(skelInfoBatchID)->GetInfo(skelInfoID);
				std::cout << "Setting skelinfo of " << Name << " to " << skelInfoBatchID << ", " << skelInfoID << '\n';
			}
		}


	protected:
		virtual unsigned int GetUIDepth() const override;
		std::vector<std::unique_ptr<MeshInstance>> MeshInstances;
		SkeletonInfo* SkelInfo;
		bool RenderAsBillboard;

		mutable glm::mat4 LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix

	};

	
}
CEREAL_REGISTER_TYPE(GEE::ModelComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GEE::Component, GEE::ModelComponent)