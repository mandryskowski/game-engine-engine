#include <scene/TextComponent.h>
#include <assetload/FileLoader.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <UI/Font.h>
#include <UI/UICanvas.h>
#include <math/Transform.h>
#include <rendering/Mesh.h>
#include <iostream>

TextComponent::TextComponent(GameScene& scene, const std::string& name, const Transform& transform, std::string content, std::shared_ptr<Font> font, std::pair<TextAlignment, TextAlignment> alignment):
	RenderableComponent(scene, name, transform),
	UsedFont(font),
	TextMatInst(nullptr),
	Content(content),
	Alignment(TextAlignment::LEFT, TextAlignment::BOTTOM)		//Set alignment at bottom-left so it's correctly aligned to alignment
{
	Material* mat = scene.GetGameHandle()->GetRenderEngineHandle()->AddMaterial(new Material("TextMaterial" + content));
	if (content == "big button")
		mat->SetColor(glm::vec4(0.7f, 0.2f, 0.6f, 1.0f));
	else
		mat->SetColor(glm::vec4(0.0f, 0.607057f, 0.663284f, 1.0f));
	TextMatInst = std::make_unique<MaterialInstance>(MaterialInstance(*mat));
	SetAlignment(alignment);	//Align to alignment
}

TextComponent::TextComponent(GameScene& scene, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment) :
	TextComponent(scene, name, transform, content, EngineDataLoader::LoadFont(*scene.GetGameHandle(), fontPath), alignment)
{
}

TextComponent::TextComponent(TextComponent&& textComp) :
	RenderableComponent(std::move(textComp)),
	Content(textComp.Content),
	UsedFont(textComp.UsedFont),
	TextMatInst(std::move(textComp.TextMatInst)),
	Alignment(textComp.Alignment)
{
}


const std::string& TextComponent::GetContent() const
{
	return Content;
}

Box2f TextComponent::GetBoundingBox(bool world) 
{
	Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
	if (CanvasPtr && world)
		transform = CanvasPtr->ToCanvasSpace(transform);

	glm::vec2 scale = transform.ScaleRef / 64.0f;

	float advancesSum = 0.0f;
	for (auto it : Content)
		advancesSum += (UsedFont->GetCharacter(it).Advance / 64.0f) * scale.x;

	glm::vec2 alignmentOffset((static_cast<float>(Alignment.first) - static_cast<float>(TextAlignment::CENTER)) * -advancesSum, (static_cast<float>(Alignment.second) - static_cast<float>(TextAlignment::CENTER)) * -transform.ScaleRef.y);

	return Box2f(glm::vec2(transform.PositionRef) + alignmentOffset, glm::vec2(advancesSum, transform.ScaleRef.y));
}

void TextComponent::SetContent(const std::string& content)
{
	Content = content;
}

void TextComponent::Render(const RenderInfo& info, Shader* shader)
{
	GameHandle->GetRenderEngineHandle()->RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), *UsedFont, Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().Color, shader, false, Alignment);
//	GameHandle->GetRenderEngineHandle()->RenderStaticMesh(RenderInfo(*GameHandle->GetRenderEngineHandle()->GetCurrentTbCollection()), MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD)), Transform(), GameHandle->GetRenderEngineHandle()->FindShader("Debug")));
	if (CanvasPtr)
		CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
}

void TextComponent::SetAlignment(const TextAlignment horizontal, const TextAlignment vertical)
{
	SetAlignment(std::pair<TextAlignment, TextAlignment>(horizontal, vertical));
}

void TextComponent::SetAlignment(const std::pair<TextAlignment, TextAlignment>& alignment)
{
	Alignment = alignment;
}