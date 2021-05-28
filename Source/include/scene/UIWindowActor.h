#pragma once
#include <UI/UICanvasActor.h>
#include <scene/UIButtonActor.h>

class UIWindowActor : public UICanvasActor
{
public:
	UIWindowActor(GameScene&, Actor* parentActor, UICanvasActor* parentCanvas, const std::string& name, const Transform& = Transform(Vec2f(0.0f), Vec2f(0.5f)));
	UIWindowActor(GameScene&, Actor* parentActor, const std::string& name);
	UIWindowActor(UIWindowActor&&);
	virtual void OnStart() override;
	/**
	 * @brief Overrides the CloseButton's OnClickFunc - func will be called upon closing the window by pressing CloseButton.
	 * Works only if CloseButton has not been set to nullptr.
	 * @param func: the function to be called upon closing the window
	*/
	void SetOnCloseFunc(std::function<void()> func);

private:
	virtual void GetExternalButtons(std::vector<UIButtonActor*>&) const override;

	UIButtonActor* CloseButton;
	UIScrollBarActor* DragButton;
};