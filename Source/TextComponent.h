#pragma once
#include "RenderableComponent.h"
#include "Material.h"

class Font;
class UICanvas;

class TextComponent: public RenderableComponent
{
public:
	TextComponent(GameScene* scene, std::string name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::shared_ptr<Font> font = nullptr);
	TextComponent(GameScene* scene, std::string name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::string fontPath = std::string());
	TextComponent(const TextComponent&);
	TextComponent(TextComponent&&);

	virtual void Render(const RenderInfo& info, Shader* shader) override;

	void AlignToCenter();

	static std::shared_ptr<TextComponent> Of(const TextComponent&);
	static std::shared_ptr<TextComponent> Of(TextComponent&&);

	void AttachToCanvas(UICanvas& canvas);

private:
	std::string Content;
	std::shared_ptr<Font> UsedFont;
	std::unique_ptr<MaterialInstance> TextMatInst;

	UICanvas* CanvasPtr;
};