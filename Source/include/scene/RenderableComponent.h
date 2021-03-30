#pragma once
#include <scene/Component.h>
#include <rendering/RenderInfo.h>

class Renderable
{
public:
	virtual void Render(const RenderInfo& info, Shader* shader) = 0;
};

class RenderableComponent: public Component, public Renderable
{
public:
	RenderableComponent::RenderableComponent(GameScene& scene, const std::string name, const Transform& transform) :
		Component(scene, name, transform)
	{
		scene.GetRenderData()->AddRenderable(*this);
	}
	RenderableComponent(RenderableComponent&& comp) :
		Component(std::move(comp))
	{
		Scene.GetRenderData()->AddRenderable(*this);
	}
	virtual void DebugRender(RenderInfo info, Shader* shader) const override {}
	virtual ~RenderableComponent()
	{
		Scene.GetRenderData()->EraseRenderable(*this);
	}
};