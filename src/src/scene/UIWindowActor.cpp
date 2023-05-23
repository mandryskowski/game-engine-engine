#include "UIWindowActor.h"
#include <game/GameScene.h>
#include <scene/UIButtonActor.h>
#include <rendering/material/Material.h>
#include <rendering/Texture.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>

namespace GEE
{
	UIWindowActor::UIWindowActor(GameScene& scene, Actor* parentActor, UICanvasActor* parentCanvas, const std::string& name, const Transform& t) :
		UICanvasActor(scene, parentActor, parentCanvas, name, t),
		CloseButton(nullptr),
		DragButton(nullptr),
		bCloseIfClickedOutside(false)
	{
	}

	UIWindowActor::UIWindowActor(GameScene& scene, Actor* parentActor, const std::string& name, const Transform& t) :
		UIWindowActor(scene, parentActor, nullptr, name, t)
	{
	}


	UIWindowActor::UIWindowActor(UIWindowActor&& actor) :
		UICanvasActor(std::move(actor)),
		CloseButton(nullptr),
		DragButton(nullptr),
		bCloseIfClickedOutside(false)
	{
		CloseButton = dynamic_cast<UIButtonActor*>(actor.FindActor("GEE_E_Close_Button"));
		DragButton = dynamic_cast<UIScrollBarActor*>(actor.FindActor("GEE_E_Drag_Button"));
	}

	void UIWindowActor::OnStart()
	{
		UICanvasActor::OnStart();

		CreateCanvasBackgroundModel();

		CloseButton = &CreateChild<UIButtonActor>("GEE_E_Close_Button");
		SetOnCloseFunc(nullptr);
		CloseButton->SetTransform(Transform(Vec2f(1.15f, 1.15f), Vec2f(0.15f)));

		auto closeIconMat = MakeShared<AtlasMaterial>("GEE_Close_Icon_Material", glm::ivec2(3, 1));
		closeIconMat->AddTexture(MakeShared<NamedTexture>(Texture::Loader<>::FromFile2D("Assets/Editor/close_icon.png", Texture::Format::RGBA(), false, Texture::MinFilter::NearestInterpolateMipmap()), "albedo1"));
		uiButtonActorUtil::ButtonMatsFromAtlas(*CloseButton, closeIconMat, 0.0f, 1.0f, 2.0f);

		DragButton = &CreateChild<UIScrollBarActor>("GEE_E_Drag_Button");
		DragButton->SetWhileBeingClickedFunc([this]() { this->GetTransform()->Move(static_cast<Vec2f>(Scene.GetUIData()->GetWindowData().GetMousePositionNDC()) - DragButton->GetClickPosNDC()); DragButton->SetClickPosNDC(Scene.GetUIData()->GetWindowData().GetMousePositionNDC()); });
		DragButton->SetTransform(Transform(Vec2f(0.0f, 1.15f), Vec2f(1.0f, 0.15f)));

		TextComponent& titleComp = DragButton->CreateComponent<TextComponent>("WindowTitle", Transform(Vec2f(-1.0f, 0.0f), Vec2f(0.15f / 1.0f, 1.0f)), GetFullCanvasName(), "", Alignment2D::LeftCenter());// .SetMaxSize(Vec2f(0.5f, 0.7f));
		titleComp.SetMaxSize(Vec2f(1.0f));
		auto titleMaterial = MakeShared<Material>("GEE_E_Title_Material");
		titleMaterial->SetColor(Vec3f(0.8f));
		titleComp.SetMaterialInst(titleMaterial);
		titleComp.Unstretch();

		DragButton->DetachFromCanvas();
		CloseButton->DetachFromCanvas();

	}

	void UIWindowActor::SetOnCloseFunc(std::function<void()> func)
	{
		if (!CloseButton)
			return;

		std::function<void()> onClick;
		if (func)
			onClick = [this, func]() { this->MarkAsKilled(); func(); };
		else
			onClick = [this]() { this->MarkAsKilled(); };

		CloseButton->SetOnClickFunc(onClick);
	}

	void UIWindowActor::SetCloseIfClickedOutside(bool close)
	{
		bCloseIfClickedOutside = close;
	}

	void UIWindowActor::KillCloseButton()
	{
		if (CloseButton)
			CloseButton->MarkAsKilled();
		CloseButton = nullptr;
	}

	void UIWindowActor::KillDragButton()
	{
		if (DragButton)
			DragButton->MarkAsKilled();
		DragButton = nullptr;
	}


	void UIWindowActor::HandleEvent(const Event& ev)
	{
		UICanvasActor::HandleEvent(ev);

		if (bCloseIfClickedOutside && ev.GetType() == EventType::MouseReleased && dynamic_cast<const MouseButtonEvent*>(&ev)->GetButton() == MouseButton::Left && !ContainsMouse())
			MarkAsKilled();
	}

	void UIWindowActor::GetExternalButtons(std::vector<UIButtonActor*>& buttons) const
	{
		UICanvasActor::GetExternalButtons(buttons);
		buttons.push_back(CloseButton);
		buttons.push_back(DragButton);
	}

}