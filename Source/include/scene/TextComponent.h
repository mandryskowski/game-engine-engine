#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Material.h>
#include <UI/UIComponent.h>
#include <UI/Font.h>

class TextComponent: public RenderableComponent, public UIComponent
{
public:
	TextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::shared_ptr<Font> font = nullptr, std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));
	TextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform &transform = Transform(), std::string content = std::string(), std::string fontPath = std::string(), std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::CENTER));
	TextComponent(const TextComponent&) = delete;
	TextComponent(TextComponent&&);

	const std::string& GetContent() const;
	virtual Boxf<Vec2f> GetBoundingBox(bool world = true) override;	//Canvas space
	float GetTextLength(bool world = true) const;

	virtual void SetContent(const std::string&);
	void SetMaterialInst(MaterialInstance&&);

	virtual void Render(const RenderInfo& info, Shader* shader) override;

	void SetHorizontalAlignment(const TextAlignment);
	void SetVerticalAlignment(const TextAlignment);
	void SetAlignment(const TextAlignment horizontal, const TextAlignment vertical = TextAlignment::TOP);	//Change what Component::ComponentTransform::Position
	void SetAlignment(const std::pair<TextAlignment, TextAlignment>& alignment);	//Change what Component::ComponentTransform::Position

	virtual void GetEditorDescription(EditorDescriptionBuilder) override;
	virtual MaterialInstance GetDebugMatInst(EditorIconState) override;

	virtual unsigned int GetUIDepth() const override;

	template <typename Archive> void Save(Archive& archive) const
	{
		std::string fontPath;
		if (UsedFont)
			fontPath = UsedFont->GetPath();
		archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", Alignment.first), cereal::make_nvp("VAlignment", Alignment.second), cereal::make_nvp("Component", cereal::base_class<Component>(this)));
	}
	template <typename Archive> void Load(Archive& archive)
	{
		std::string fontPath;
		archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", Alignment.first), cereal::make_nvp("VAlignment", Alignment.second), cereal::make_nvp("Component", cereal::base_class<Component>(this)));

		if (!fontPath.empty())
			UsedFont = EngineDataLoader::LoadFont(*GameHandle, fontPath);
	}

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


class TextConstantSizeComponent: public TextComponent
{
public:
	TextConstantSizeComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), std::string content = std::string(), std::shared_ptr<Font> font = nullptr, std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM));
	TextConstantSizeComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), std::string content = std::string(), std::string fontPath = std::string(), std::pair<TextAlignment, TextAlignment> = std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, TextAlignment::BOTTOM));
	TextConstantSizeComponent(const TextConstantSizeComponent&) = delete;
	TextConstantSizeComponent(TextConstantSizeComponent&&);
	void SetMaxSize(const glm::vec2&);
	virtual void SetContent(const std::string&) override;
	void UpdateSize();
private:
	glm::vec2 MaxSize;
	glm::vec2 ScaleRatio;
};