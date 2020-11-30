#pragma once
#include "Component.h"
#include "RenderInfo.h"

class RenderableComponent: public Component
{
public:
	using Component::Component;
	virtual void Render(RenderInfo& info, Shader* shader) = 0;
	virtual void DebugRender(RenderInfo info, Shader* shader) const override {}
};

