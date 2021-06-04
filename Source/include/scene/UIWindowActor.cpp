#include "UIWindowActor.h"
#include <scene/UIButtonActor.h>
#include <rendering/Material.h>
#include <rendering/Texture.h>
#include <input/InputDevicesStateRetriever.h>
#include <scene/TextComponent.h>
#include <scene/ModelComponent.h>

namespace GEE
{
	UIWindowActor::UIWindowActor(GameScene& scene, Actor* parentActor, UICanvasActor* parentCanvas, const std::string& name, const Transform& t) :
		UICanvasActor(scene, parentActor, parentCanvas, name, t),
		CloseButton(nullptr),
		DragButton(nullptr)
	{
	}

	UIWindowActor::UIWindowActor(GameScene& scene, Actor* parentActor, const std::string& name) :
		UIWindowActor(scene, parentActor, nullptr, name)
	{
	}


	UIWindowActor::UIWindowActor(UIWindowActor&& actor) :
		UICanvasActor(std::move(actor)),
		CloseButton(nullptr),
		DragButton(nullptr)
	{
		CloseButton = dynamic_cast<UIButtonActor*>(actor.FindActor("GEE_E_Close_Button"));
		DragButton = dynamic_cast<UIScrollBarActor*>(actor.FindActor("GEE_E_Drag_Button"));
	}

	void UIWindowActor::OnStart()
	{
		UICanvasActor::OnStart();

		ModelComponent& backgroundModel = CreateComponent<ModelComponent>("WindowBackground");
		backgroundModel.AddMeshInst(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		backgroundModel.OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_E_Canvas_Background_Material").get());

		AddUIElement((UICanvasElement&)backgroundModel);	//Add the background model as a UIElement to give it UIDepth
		EraseUIElement((UICanvasElement&)backgroundModel);	//Erase it immediately because we do not want it to be an element of the canvas, but its background

		CloseButton = &CreateChild<UIButtonActor>("GEE_E_Close_Button");
		SetOnCloseFunc(nullptr);
		CloseButton->SetTransform(Transform(Vec2f(1.15f, 1.15f), Vec2f(0.15f)));

		AtlasMaterial& closeIconMat = *new AtlasMaterial(Material("GEE_Close_Icon_Material", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(3, 1));
		closeIconMat.AddTexture(std::make_shared<NamedTexture>(Texture::Loader::FromFile2D("EditorAssets/close_icon.png", false, Texture::MinTextureFilter::NearestInterpolateMipmap()), "albedo1"));
		CloseButton->SetMatIdle(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(0.0f)));
		CloseButton->SetMatHover(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
		CloseButton->SetMatClick(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

		DragButton = &CreateChild<UIScrollBarActor>("GEE_E_Drag_Button");
		DragButton->SetWhileBeingClickedFunc([this]() { this->GetTransform()->Move(static_cast<Vec2f>(GameHandle->GetInputRetriever().GetMousePositionNDC()) - DragButton->GetClickPosNDC()); DragButton->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC()); });
		DragButton->SetTransform(Transform(Vec2f(0.0f, 1.15f), Vec2f(1.0f, 0.15f)));

		TextConstantSizeComponent& titleComp = DragButton->CreateComponent<TextConstantSizeComponent>("WindowTitle", Transform(Vec2f(-1.0f, 0.0f), Vec2f(0.15f / 1.0f, 1.0f)), GetFullCanvasName(), "", std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));// .SetMaxSize(Vec2f(0.5f, 0.7f));
		Material& titleMaterial = *new Material(Material("GEE_E_Title_Material", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight")));
		titleMaterial.SetColor(Vec3f(0.8f));
		titleComp.SetMaterialInst(titleMaterial);

		EraseUIElement(*DragButton);
		EraseUIElement(*CloseButton);

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

	void UIWindowActor::GetExternalButtons(std::vector<UIButtonActor*>& buttons) const
	{
		UICanvasActor::GetExternalButtons(buttons);
		buttons.push_back(CloseButton);
		buttons.push_back(DragButton);
	}

}