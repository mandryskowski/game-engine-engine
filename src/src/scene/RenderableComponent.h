#pragma once
#include <scene/Component.h>
#include <rendering/RenderInfo.h>
namespace GEE
{
	class Renderable
	{
	public:
		Renderable(GameScene& scene);
		Renderable(Renderable&& renderable);
		virtual void Render(const SceneMatrixInfo& info, Shader* shader) = 0;
		bool GetHide() const;
		void SetHide(bool hide);
		bool CastsShadow() const;
		void SetCastsShadow(bool castsShadow);
		virtual std::vector<const Material*> GetMaterials() const;
		virtual ~Renderable();
	protected:
		virtual unsigned int GetUIDepth() const { return 0; }
		bool Hide;
		bool bCastsShadow;
		GameSceneRenderData& SceneRenderData;
		friend class RenderEngine;
		friend class GameSceneRenderData;
		friend class UIComponent;
	};


	class RenderableComponent : public Component, public Renderable
	{
	public:
		RenderableComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform) :
			Component(actor, parentComp, name, transform),
			Renderable(GetScene()) {}
		RenderableComponent(RenderableComponent&& comp) :
			Component(std::move(comp)),
			Renderable(std::move(comp)) {}
		void DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale) const override {}
		void GetEditorDescription(ComponentDescriptionBuilder) override;
		template <typename Archive>
		void Save(Archive& archive) const
		{
			archive(cereal::make_nvp("Hide", GetHide()), cereal::make_nvp("CastsShadow", CastsShadow()), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
		}
		template <typename Archive>
		void Load(Archive& archive)
		{
			bool hide = false, castsShadow = true;
			archive(cereal::make_nvp("Hide", hide), cereal::make_nvp("CastsShadow", castsShadow), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
			SetHide(hide);
			SetCastsShadow(castsShadow);
		}
		virtual ~RenderableComponent() = default;
	protected:
	};
}