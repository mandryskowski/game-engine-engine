#pragma once
#include <UI/UICanvasActor.h>
#include <scene/UIButtonActor.h>

class UIWindowActor : public UICanvasActor
{
public:
	UIWindowActor(GameScene&, const std::string& name);
	UIWindowActor(UIWindowActor&&);
	virtual void OnStart() override;

};