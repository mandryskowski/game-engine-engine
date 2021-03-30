#include "UIWindowActor.h"
#include <scene/UIButtonActor.h>
#include <rendering/Material.h>
#include <rendering/Texture.h>

UIWindowActor::UIWindowActor(GameScene& scene, const std::string& name):
	UICanvasActor(scene, name)
{
}

UIWindowActor::UIWindowActor(UIWindowActor&& actor):
	UICanvasActor(std::move(actor))
{
}
#include <input/InputDevicesStateRetriever.h>

void UIWindowActor::OnStart()
{
	UICanvasActor::OnStart();
	UIButtonActor& closeButton = CreateChild(UIButtonActor(Scene, Name + "'s close button", [this]() {this->MarkAsKilled(); }));
	closeButton.SetTransform(Transform(glm::vec2(1.15f, 1.15f), glm::vec2(0.15f)));

	AtlasMaterial& closeIconMat = *new AtlasMaterial("GEE_Close_Icon_Material", glm::ivec2(3, 1), 0.0f, 0.0f, GameHandle->GetRenderEngineHandle()->FindShader("Forward_NoLight"));
	closeIconMat.AddTexture(new NamedTexture(textureFromFile("EditorAssets/close_icon.png", GL_RGB, GL_LINEAR, GL_NEAREST_MIPMAP_LINEAR), "albedo1"));
	closeButton.SetMatIdle(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(0.0f)));
	closeButton.SetMatHover(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(1.0f)));
	closeButton.SetMatClick(MaterialInstance(closeIconMat, closeIconMat.GetTextureIDInterpolatorTemplate(2.0f)));

	UIScrollBarActor& dragButton = CreateChild(UIScrollBarActor(Scene, "DragButton"));
	dragButton.SetWhileBeingClickedFunc([&dragButton, this]() { this->GetTransform()->Move(static_cast<glm::vec2>(GameHandle->GetInputRetriever().GetMousePositionNDC()) - dragButton.GetClickPosNDC()); dragButton.SetClickPosNDC(GameHandle->GetInputRetriever().GetMousePositionNDC());});
	dragButton.SetTransform(Transform(glm::vec2(0.0f, 1.15f), glm::vec2(1.0f, 0.15f)));



	//AddUIElement(closeButton);
}