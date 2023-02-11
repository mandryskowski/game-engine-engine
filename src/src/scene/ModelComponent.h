#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Mesh.h>
#include "physics/CollisionObject.h"
#include <UI/UIComponent.h>
#include <animation/SkeletonInfo.h>
#include <scene/hierarchy/HierarchyNodeInstantiation.h>

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
		friend class Hierarchy::Node<ModelComponent>;
		ModelComponent& operator=(const ModelComponent&);
	public:
		ModelComponent& operator=(ModelComponent&&) = delete;	//TODO: de-delete this, it should be written but i am too lazy

		void OverrideInstancesMaterial(SharedPtr<Material> material);	//Overrides MeshInstances' materials with the one passed as the argument
		void OverrideInstancesMaterialInstances(SharedPtr<MaterialInstance> matInst);	//Overrides MeshInstances' materials with the one passed as the argument
		void SetLastFrameMVP(const Mat4f& lastMVP) const;
		void SetSkeletonInfo(SkeletonInfo*);
		void SetRenderAsBillboard(bool billboard);

		void DRAWBATCH() const;

		unsigned GetMeshInstanceCount() const;
		const MeshInstance& GetMeshInstance(int index) const;
		const Mat4f& GetLastFrameMVP() const;
		SkeletonInfo* GetSkeletonInfo() const;
		std::vector<const Material*> GetMaterials() const override;

		MeshInstance* FindMeshInstance(const std::string& nodeName, const std::string& specificMeshName = std::string());
		void AddMeshInst(const MeshInstance&);
		void AddMeshInst(MeshInstance&&);

		void Update(Time dt) override;

		void Render(const SceneMatrixInfo&, Shader* shader) override;

		void GetEditorDescription(ComponentDescriptionBuilder) override;

		/*virtual void DebugRender(RenderInfo info, Shader* shader) const override
		{
			Transform world = GetTransform().GetWorldTransform();
			//world.SetRotation(Vec3f(0.0f));	// ignore rotation (we want boxes to be axis-aligned)
			Material colorMat("AAAAAA", 0.0f, shader);
			colorMat.SetColor(Vec4f(1.0f));

			for (auto& it : MeshInstances)
			{
				Transform transform = world * Transform(it->GetMesh().GetBoundingBox().Position, Quatf(Vec3f(0.0f)), it->GetMesh().GetBoundingBox().Size);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				GameHandle->GetRenderEngineHandle()->RenderStaticMesh(info, MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::Cube), &colorMat), transform, shader, nullptr, nullptr);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				colorMat.SetColor(colorMat.GetColor() - 0.1f);
			}
		}*/

		template <typename Archive> void Save(Archive& archive) const;
		template <typename Archive> void Load(Archive& archive);

		virtual ~ModelComponent();
	protected:
		unsigned int GetUIDepth() const override;
		void SignalSkeletonInfoDeath();

		friend class SkeletonInfo;

	public: //navmesh
		std::vector<UniquePtr<MeshInstance>> MeshInstances;
		SkeletonInfo* SkelInfo;
		bool RenderAsBillboard;

		mutable Mat4f LastFrameMVP;	//for velocity buffer; this field is only updated when velocity buffer is needed (for temporal AA/motion blur), in any other case it will be set to an identity matrix
	};

	template <> inline void Hierarchy::Instantiation::Impl<ModelComponent>::InstantiateToComp(SharedPtr<Instantiation::Data> data, const NodeBase& instantiatedNode, ModelComponent& targetComp)
	{
		Impl<Component>::InstantiateToComp(data, instantiatedNode, targetComp);

		if (data->GetSkeletonInfo())
			targetComp.SetSkeletonInfo(data->GetSkeletonInfo());
	}
}
GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::ModelComponent)