#include "TextComponent.h"
#include "FileLoader.h"
#include "Mesh.h"	//for RenderInfo
#include "Font.h"
#include "UICanvasActor.h"

TextComponent::TextComponent(GameScene* scene, std::string name, const Transform& transform, std::string content, std::shared_ptr<Font> font):
	RenderableComponent(scene, name, transform),
	Content(content),
	UsedFont(font),
	TextMatInst(nullptr),
	CanvasPtr(nullptr)
{
	Material* mat = scene->GetGameHandle()->GetRenderEngineHandle()->AddMaterial(new Material("TextMaterial" + content));
	if (content == "big button")
		mat->SetColor(glm::vec4(0.7f, 0.2f, 0.6f, 1.0f));
	else
		mat->SetColor(glm::vec4(0.0f, 0.73f, 0.84f, 1.0f));
	TextMatInst = std::make_unique<MaterialInstance>(MaterialInstance(*mat));
}

TextComponent::TextComponent(GameScene* scene, std::string name, const Transform &transform, std::string content, std::string fontPath):
	TextComponent(scene, name, transform, content, EngineDataLoader::LoadFont(fontPath))
{
}

TextComponent::TextComponent(const TextComponent& textComp) :
	TextComponent(textComp.Scene, textComp.Name, textComp.ComponentTransform, textComp.Content, textComp.UsedFont)
{
}

TextComponent::TextComponent(TextComponent&& textComp) :
	TextComponent(textComp.Scene, textComp.Name, textComp.ComponentTransform, textComp.Content, textComp.UsedFont)
{
}

void TextComponent::Render(const RenderInfo& info, Shader* shader)
{
	if (CanvasPtr)
	{
		RenderInfo convertedInfo = info;
		convertedInfo.view = CanvasPtr->GetView();
		convertedInfo.projection = CanvasPtr->GetProjection();
		convertedInfo.CalculateVP();
		CanvasPtr->GetViewport().SetOpenGLState(GameHandle->GetGameSettings()->WindowSize);
		GameHandle->GetRenderEngineHandle()->RenderText(convertedInfo, *UsedFont, Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().Color, shader);
		Viewport(glm::uvec2(0), GameHandle->GetGameSettings()->WindowSize).SetOpenGLState();
	}
	else
		GameHandle->GetRenderEngineHandle()->RenderText(info, *UsedFont, Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().Color, shader);
}

void TextComponent::AlignToCenter()
{
	float advancesSum = 0.0f;
	for (int i = 0; i < static_cast<int>(Content.length()); i++)
		advancesSum += (UsedFont->GetCharacter(Content[i]).Advance / 64.0f) * GetTransform().ScaleRef.x / 2.0f;
	std::cout << "Advances sum: " << advancesSum / 64.0f << "\n";
	GetTransform().Move(glm::vec3(-advancesSum / 64.0f, -GetTransform().ScaleRef.y / 2.0f, 0.0f));
}

std::shared_ptr<TextComponent> TextComponent::Of(const TextComponent& constructorVal)
{
	std::shared_ptr<TextComponent> createdObj = std::make_shared<TextComponent>(TextComponent(constructorVal));
	constructorVal.Scene->GetRenderData()->AddRenderable(createdObj);
	return createdObj;
}

std::shared_ptr<TextComponent> TextComponent::Of(TextComponent&& constructorVal)
{
	std::shared_ptr<TextComponent> createdObj = std::make_shared<TextComponent>(TextComponent(constructorVal));
	constructorVal.Scene->GetRenderData()->AddRenderable(createdObj);
	return createdObj;
}

void TextComponent::AttachToCanvas(UICanvas& canvas)
{
	CanvasPtr = &canvas;
}
