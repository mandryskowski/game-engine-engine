#pragma once
#include <scene/Component.h>
#include <rendering/RenderInfo.h>
namespace GEE
{
	class Renderable
	{
	public:
		Renderable(GameScene& scene) : SceneRenderData(*scene.GetRenderData()), Hide(false), bCastsShadow(true) { SceneRenderData.AddRenderable(*this); }
		Renderable(Renderable&& renderable) : SceneRenderData(renderable.SceneRenderData), Hide(renderable.Hide), bCastsShadow(renderable.bCastsShadow) { SceneRenderData.AddRenderable(*this); }
		virtual void Render(const SceneMatrixInfo& info, Shader* shader) = 0;
		bool GetHide() const { return Hide; }
		void SetHide(bool hide) { Hide = hide; }
		bool CastsShadow() const { return bCastsShadow; }
		void SetCastsShadow(bool castsShadow) { bCastsShadow = castsShadow; }
		virtual std::vector<const Material*> GetMaterials() const { return { nullptr }; }
		virtual ~Renderable() { SceneRenderData.EraseRenderable(*this); }
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
			Renderable(Scene) {}
		RenderableComponent(RenderableComponent&& comp) :
			Component(std::move(comp)),
			Renderable(std::move(comp)) {}
		virtual void DebugRender(SceneMatrixInfo info, Shader& shader, const Vec3f& debugIconScale) const override {}
		virtual void GetEditorDescription(EditorDescriptionBuilder) override;
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