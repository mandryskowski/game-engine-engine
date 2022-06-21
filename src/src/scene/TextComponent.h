#pragma once
#include <scene/RenderableComponent.h>
#include <rendering/Material.h>
#include <UI/UIComponent.h>
#include <UI/Font.h>
#include <utility/Alignment.h>
namespace GEE
{
	enum class UISpace
	{
		Local,
		World,
		Canvas,
		Window
	};


	class TextComponent : public RenderableComponent, public UIComponent
	{
	public:
		TextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), const std::string& content = std::string(), SharedPtr<Font> font = nullptr, Alignment2D = Alignment2D::LeftCenter());
		TextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), const std::string& content = std::string(), const std::string& fontPath = std::string(), Alignment2D = Alignment2D::LeftCenter());
		TextComponent(const TextComponent&) = delete;
		TextComponent(TextComponent&&);

		const std::string& GetContent() const;
		virtual Boxf<Vec2f> GetBoundingBox(bool world = true) const override;	//Canvas space
		float GetTextLength(bool world = true) const;
		float GetTextHeight(bool world = true) const;
		Vec2f GetTextMaxVerticalExtents(bool world = true) const;
		virtual std::vector<const Material*> GetMaterials() const override;
		Font* GetFont();
		FontStyle GetFontStyle() const;
		Font::Variation* GetFontVariation();
		const Font::Variation* GetFontVariation() const;
		MaterialInstance* GetTextMatInst();
		virtual Alignment2D GetAlignment() const;

		bool IsScrollable() const;
		Transform GetTransformCorrectedForSize(bool world = true) const;
		void SetMaxSize(const Vec2f& maxSize) { MaxSize = maxSize; Unstretch(); }
		/**
		 * @brief Recalculates the ScaleRatio based on the scale of this Component's Transform.
	 	 * @param space: The space to be used in order to compute ScaleRatio. You can use local/world transform, canvas and window space.
		*/
		void RecalculateScaleRatio(UISpace space = UISpace::Window);
		

		
		virtual void SetContent(const std::string&);
		void SetMaterialInst(MaterialInstance&&);
		void SetFont(SharedPtr<Font> font) { _Font = font; }
		void SetFont(const std::string& fontPath);
		void SetFontStyle(FontStyle style) { _FontStyle = style; }

		virtual void Render(const SceneMatrixInfo& info, Shader* shader) override;

		void SetHorizontalAlignment(Alignment);
		void SetVerticalAlignment(Alignment);
		void SetAlignment(Alignment2D);

		virtual void HandleEvent(const Event& ev) override;
		/**
		 * @brief Unstretch the text (make the scale uniform) based on this Component's Transform. The scale X and Y will be set to the smallest component of the two.
		 * @param space: The space to be used in order to compute ScaleRatio. You can use local/world transform, canvas and window space.
		*/
		virtual void Unstretch(UISpace space = UISpace::Window);

		virtual void GetEditorDescription(ComponentDescriptionBuilder) override;
		virtual	MaterialInstance GetDebugMatInst(ButtonMaterialType) override;

		virtual unsigned int GetUIDepth() const override;

		template <typename Archive> void Save(Archive& archive) const
		{
			std::string fontPath;
			if (_Font)
				fontPath = _Font->GetVariation(FontStyle::Regular)->GetPath();
			archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", _Alignment.GetHorizontal()), cereal::make_nvp("VAlignment", _Alignment.GetVertical()), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			std::string fontPath;
			Alignment alignmentH, alignmentV;
			archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", alignmentH), cereal::make_nvp("VAlignment", alignmentV), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
			SetAlignment(Alignment2D(alignmentH, alignmentV));

			if (!fontPath.empty())
				_Font = EngineDataLoader::LoadFont(*GameHandle, fontPath);
		}

	private:
		std::string Content;
		SharedPtr<Font> _Font;
		UniquePtr<MaterialInstance> TextMatInst;

		/*Vertical and horziontal alignment. Alignment is in relation to Component::ComponentTransform::Position
		  Default alignment is Left and Center. This means that Component::ComponentTransform::Position marks the left-center point of the text.
		  CENTER alignment means that the TextComponent is  centred around Component::ComponentTransform::Position.
		  RIGHT means that the text is moved during rendering so it ends at Component::ComponentTransform::Position and TOP that the next never goes above previous Component::ComponentTransform::Position::y.
		  Note that Component::ComponentTransform::Position is not always at the bottom-left corner of the text.
		  Use a function (that DOES NOT EXIST YET. TODO) to get the bottom-left corner of the text.
		*/
		Alignment2D _Alignment;					

		Vec2f MinSize, MaxSize;
		Vec2f ScaleRatio;

		FontStyle _FontStyle;
	};

	class ScrollingTextComponent : public TextComponent
	{
	public:
		ScrollingTextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), std::string content = std::string(), SharedPtr<Font> font = nullptr, Alignment2D = Alignment2D::LeftCenter());
		ScrollingTextComponent(Actor&, Component* parentComp, const std::string& name = std::string(), const Transform& transform = Transform(), std::string content = std::string(), std::string fontPath = std::string(), Alignment2D = Alignment2D::LeftCenter());

		bool IsScrollable() const;

		void ScrollToLetter(unsigned int letterIndex);

		virtual Boxf<Vec2f> GetBoundingBox(bool world = true) const override;	//Canvas space
		virtual Alignment2D GetAlignment() const override;

		Transform GetCorrectedTransform(bool world = true);

		virtual void Update(float deltaTime);
		virtual void Render(const SceneMatrixInfo& info, Shader* shader) override;

	protected:

		float TimeSinceScrollReset, ScrollResetTime, ScrollCooldownTime;
		Interpolation ScrollingInterp;
	};

	namespace textUtil
	{
		float GetTextLength(const std::string& str, const Vec2f& textScale, const Font::Variation&);
		float GetTextHeight(const std::string& str, const Vec2f& textScale, const Font::Variation&);
		Vec2f GetTextMaxVerticalExtents(const std::string& str, const Vec2f& textScale, const Font::Variation&);

		Boxf<Vec2f> ComputeBBox(const std::string& str, const Transform& spaceTransform, const Font::Variation&, Alignment2D);

		Vec2f ComputeScale(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio = Vec2f(1.0f));
		Vec2f ComputeScaleRatio(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio = Vec2f(1.0f));

		Transform OwnedToSpaceTransform(UISpace space, const Transform& ownedTransform, const UICanvas* canvas, const WindowData* windowData);

	}
}
GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::TextComponent, GEE::Transform(), "", "")