#include "UIWindowActor.h"
#include <scene/UIButtonActor.h>
#include <rendering/Material.h>
#include <rendering/Texture.h>

UIWindowActor::UIWindowActor(GameScene& scene, Actor* parentActor, const std::string& name):
	UICanvasActor(scene, parentActor, name),
	CloseButton(nullptr),
	DragButton(nullptr)
{
}

UIWindowActor::UIWindowActor(UIWindowActor&& actor):
	UICanvasActor(std::move(actor)),
	CloseButton(nullptr),
	DragButton(nullptr)
{
	CloseButton = dynamic_cast<UIButtonActor*>(actor.FindActor("GEE_E_Close_Button"));
	DragButton = dynamic_cast<UIScrollBarActor*>(actor.FindActor("GEE_E_Drag_Button"));
}
#include <input/InputDevicesStateRetriever.h>

void UIWindowActor::OnStart()
{
	UICanvasActor::OnStart();
	CloseButton = &CreateChild<UIButtonActor>("GEE_E_Close_Button", [this]() {this->MarkAsKilled(); });
	CloseButton->SetTransform(Transform(glm::vec2(1.15f, 1.15f), glm::vec2(0.15f)));

	AtlasMaterial& closeIconMat = *new AtlasMaterial(Material("GEE_Close_Icon_Material", 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight")), glm::ivec2(3, 1));
	closeIconMat.AddTexture(std::make_shared<NamedTexture>(textureFromFile("EditorAssets/close_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
	CloseButton->SetMatIdle(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(0.0f)));
	CloseButton->SetMatHover(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
	CloseButton->SetMatClick(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

	DragButton = &CreateChild<UIScrollBarActor>("GEE_E_Drag_Button");
	DragButton->SetWhileBeingClickedFunc([this]() { this->GetTransform()->Move(static_cast<glm::vec2>(GameHandle->GetInputRetriever().GetMousePositionNDC()) - DragButton->GetClickPosNDC()); DragButton->SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());});
	DragButton->SetTransform(Transform(glm::vec2(0.0f, 1.15f), glm::vec2(1.0f, 0.15f)));

	EraseUIElement(*DragButton);
	EraseUIElement(*CloseButton);

}

void UIWindowActor::GetExternalButtons(std::vector<UIButtonActor*>& buttons) const
{
	UICanvasActor::GetExternalButtons(buttons);
	buttons.push_back(CloseButton);
	buttons.push_back(DragButton);
}
