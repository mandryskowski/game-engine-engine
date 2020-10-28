#pragma once
#include "Component.h"
class RenderableComponent: public Component
{
public:
	using Component::Component;
	virtual void Render(RenderInfo& info, Shader* shader) = 0;
};

