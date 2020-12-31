#pragma once
#include "Actor.h"
#include "RenderableComponent.h"
#include "UIModelComponent.h"
#include "Viewport.h"

class UICanvas
{
public:
	virtual NDCViewport GetViewport() const = 0;
	virtual glm::mat4 GetView() const = 0;
	virtual glm::mat4 GetProjection() const;

	void ScrollView(glm::vec2 offset);

protected:
	Transform CanvasView;
};
 

class UICanvasActor : public Actor, public UICanvas
{
public:
	UICanvasActor(GameScene*, const std::string& name);

	virtual glm::mat4 GetView() const override;
	virtual NDCViewport GetViewport() const override;

	virtual void HandleEvent(const Event& ev) override;
};

