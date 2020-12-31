#pragma once
#include "Component.h"
#include "RenderInfo.h"

class Renderable
{
public:
	virtual void Render(const RenderInfo& info, Shader* shader) = 0;
};

class RenderableComponent: public Component, public Renderable
{
public:
	using Component::Component;
	virtual void DebugRender(RenderInfo info, Shader* shader) const override {}
};

