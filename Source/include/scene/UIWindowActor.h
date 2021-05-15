#pragma once
#include <UI/UICanvasActor.h>
#include <scene/UIButtonActor.h>

class UIWindowActor : public UICanvasActor
{
public:
	UIWindowActor(GameScene&, Actor* parentActor, const std::string& name);
	UIWindowActor(UIWindowActor&&);
	virtual void OnStart() override;

private:
	virtual void GetExternalButtons(std::vector<UIButtonActor*>&) const override;

	UIButtonActor* CloseButton;
	UIScrollBarActor* DragButton;
};