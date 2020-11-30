#pragma once
#include "RenderableComponent.h"

class Font;

class TextComponent: public RenderableComponent
{
	std::string Content;
	glm::vec3 Color;
	std::shared_ptr<Font> UsedFont;

public:
	TextComponent(GameScene* scene, std::string name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::shared_ptr<Font> font = nullptr);
	TextComponent(GameScene* scene, std::string name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::string fontPath = std::string());
	TextComponent(const TextComponent&);
	TextComponent(TextComponent&&);
	virtual void Render(RenderInfo& info, Shader* shader) override;

	static std::shared_ptr<TextComponent> Of(const TextComponent&);
	static std::shared_ptr<TextComponent> Of(TextComponent&&);
};