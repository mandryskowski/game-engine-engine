#include "TextComponent.h"
#include "FileLoader.h"
#include "Mesh.h"	//for RenderInfo
#include "Font.h"

TextComponent::TextComponent(GameManager* gameHandle, std::string name, const Transform& transform, std::string content, std::shared_ptr<Font> font):
	RenderableComponent(gameHandle, name, transform),
	Content(content),
	UsedFont(font)
{
}

TextComponent::TextComponent(GameManager* gameHandle, std::string name, const Transform &transform, std::string content, std::string fontPath):
	TextComponent(gameHandle, name, transform, content, EngineDataLoader::LoadFont(fontPath))
{
}

TextComponent::TextComponent(const TextComponent& textComp) :
	TextComponent(textComp.GameHandle, textComp.Name, textComp.ComponentTransform, textComp.Content, textComp.UsedFont)
{
}

TextComponent::TextComponent(TextComponent&& textComp) :
	TextComponent(textComp.GameHandle, textComp.Name, textComp.ComponentTransform, textComp.Content, textComp.UsedFont)
{
}

void TextComponent::Render(RenderInfo& info, Shader* shader)
{
	GameHandle->GetRenderEngineHandle()->RenderText(info, *UsedFont, Content, GetTransform().GetWorldTransform(), glm::vec3(0.0f, 0.73f, 0.84f), shader);
}

std::shared_ptr<TextComponent> TextComponent::Of(const TextComponent& constructorVal)
{
	std::shared_ptr<TextComponent> createdObj = std::make_shared<TextComponent>(TextComponent(constructorVal));
	constructorVal.GameHandle->GetRenderEngineHandle()->AddRenderable(createdObj);
	return createdObj;
}

std::shared_ptr<TextComponent> TextComponent::Of(TextComponent&& constructorVal)
{
	std::shared_ptr<TextComponent> createdObj = std::make_shared<TextComponent>(TextComponent(constructorVal));
	constructorVal.GameHandle->GetRenderEngineHandle()->AddRenderable(createdObj);
	return createdObj;
}
