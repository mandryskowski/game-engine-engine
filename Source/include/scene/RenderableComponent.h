#pragma once
#include <scene/Component.h>
#include <rendering/RenderInfo.h>
namespace GEE
{
	class Renderable
	{
	public:
		Renderable(GameScene& scene) : SceneRenderData(*scene.GetRenderData()), Hide(false) { SceneRenderData.AddRenderable(*this); }
		Renderable(Renderable&& renderable) : SceneRenderData(renderable.SceneRenderData), Hide(renderable.Hide) { SceneRenderData.AddRenderable(*this); }
		virtual void Render(const RenderInfo& info, Shader* shader) = 0;
		bool GetHide() const { return Hide; }
		void SetHide(bool hide) { Hide = hide; }
		~Renderable() { SceneRenderData.EraseRenderable(*this); }
	protected:
		virtual unsigned int GetUIDepth() const { return 0; }
		bool Hide;
		GameSceneRenderData& SceneRenderData;
		friend class RenderEngine;
		friend class GameSceneRenderData;
		friend class UIComponent;
	};


	class RenderableComponent : public Component, public Renderable
	{
	public:
		RenderableComponent(Actor& actor, Component* parentComp, const std::string name, const Transform& transform) :
			Component(actor, parentComp, name, transform),
			Renderable(Scene) {}
		RenderableComponent(RenderableComponent&& comp) :
			Component(std::move(comp)),
			Renderable(std::move(comp)) {}
		virtual void DebugRender(RenderInfo info, Shader* shader) const override {}
	protected:
	};
}