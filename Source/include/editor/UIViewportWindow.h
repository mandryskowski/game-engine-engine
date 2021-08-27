#pragma once
#include <scene/UIWindowActor.h>

namespace GEE
{
	class UIViewportWindow : public UIWindowActor
	{
	public:
		UIViewportWindow(GameScene&, Actor* parentActor, UICanvasActor* parentCanvas, const std::string& name, const Transform & = Transform(Vec2f(0.0f), Vec2f(0.5f)));
		UIViewportWindow(GameScene&, Actor* parentActor, const std::string& name);
	};



}