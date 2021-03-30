#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Material.h>
#include <UI/UIElement.h>

class Font;

class TextComponent: public RenderableComponent, public UICanvasElement
{
public:
	TextComponent(GameScene& scene, const std::string& name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::shared_ptr<Font> font = nullptr, std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM));
	TextComponent(GameScene& scene, const std::string& name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::string fontPath = std::string(), std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM));
	TextComponent(const TextComponent&) = delete;
	TextComponent(TextComponent&&);

	const std::string& GetContent() const;
	virtual Box2f GetBoundingBox(bool world = true) override;	//Canvas space

	void SetContent(const std::string&);

	virtual void Render(const RenderInfo& info, Shader* shader) override;

	void SetAlignment(const TextAlignment horizontal, const TextAlignment vertical = TextAlignment::TOP);	//Change what Component::ComponentTransform::Position
	void SetAlignment(const std::pair<TextAlignment, TextAlignment>& alignment);	//Change what Component::ComponentTransform::Position

private:
	std::string Content;
	std::shared_ptr<Font> UsedFont;
	std::unique_ptr<MaterialInstance> TextMatInst;

	std::pair<TextAlignment, TextAlignment> Alignment;		/*Vertical and horziontal alignment. Alignment is in relation to Component::ComponentTransform::Position
															  Default alignment is LEFT and BOTTOM. This means that Component::ComponentTransform::Position marks the bottom-left corner of the text.
															  CENTER alignment means that the TextComponent is  centred around Component::ComponentTransform::Position.
															  RIGHT means that the text is moved during rendering so it ends at Component::ComponentTransform::Position and TOP that the next never goes above previous Component::ComponentTransform::Position::y.
															  Note that Component::ComponentTransform::Position is not always at the bottom-left corner of the text.
															  Use a function (that DOES NOT EXIST YET. TODO) to get the bottom-left corner of the text.
															*/
};