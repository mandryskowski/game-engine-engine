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
	RenderableComponent::RenderableComponent(Actor& actor, Component* parentComp, const std::string name, const Transform& transform) :
		Component(actor, parentComp, name, transform),
		Hide(false)
	{
		if (Name == "Tunnel")
			std::cout << "Dodaje z constructora\n";
		Scene.GetRenderData()->AddRenderable(*this);
	}
	RenderableComponent(RenderableComponent&& comp) :
		Component(std::move(comp))
	{
		if (Name == "Tunnel")
			std::cout << "Dodaje z move\n";
		Scene.GetRenderData()->AddRenderable(*this);
	}
	virtual void DebugRender(RenderInfo info, Shader* shader) const override {}
	bool GetHide() const { return Hide; }
	void SetHide(bool hide) { Hide = hide; }
	virtual ~RenderableComponent()
	{
		Scene.GetRenderData()->EraseRenderable(*this);
	}
protected:
	bool Hide;
};