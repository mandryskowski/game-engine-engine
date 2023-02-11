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
		TextComponent(Actor&, Component* parentComp, const String& name = String(), const Transform& transform = Transform(), const String& content = String(), SharedPtr<Font> font = nullptr, Alignment2D = Alignment2D::LeftCenter());
		TextComponent(Actor&, Component* parentComp, const String& name = String(), const Transform& transform = Transform(), const String& content = String(), const String& fontPath = String(), Alignment2D = Alignment2D::LeftCenter());
		TextComponent(const TextComponent&) = delete;
		TextComponent(TextComponent&&);

		void OnStart() override
		{
			RenderableComponent::OnStart();
			TransformDirtyFlag = static_cast<int>(GetTransform().AddDirtyFlag());
		}

		String GetContent() const;
		Boxf<Vec2f> GetBoundingBox(bool world = true) const override;	//Canvas space
		float GetTextLength(bool world = true) const;
		float GetTextHeight(bool world = true) const;
		Vec2f GetTextMaxVerticalExtents(bool world = true) const;
		std::vector<const Material*> GetMaterials() const override;

		// Font getters
		Font* GetFont()
		{
			return _Font.get();
		}

		FontStyle GetFontStyle() const
		{
			return _FontStyle;
		}

		Font::Variation* GetFontVariation()
		{
			return _Font->GetVariation(_FontStyle);
		}

		const Font::Variation* GetFontVariation() const
		{
			return _Font->GetVariation(_FontStyle);
		}



		MaterialInstance* GetTextMatInst()
		{
			return TextMatInst.get();
		}

		virtual Alignment2D GetAlignment() const
		{
			return _Alignment;
		}



		virtual bool IsScrollable() const;
		Transform GetTransformCorrectedForSize(UISpace) const;
		void SetMaxSize(const Vec2f& maxSize) { MaxSize = maxSize; Unstretch(); }

		/**
		 * @brief Recalculates the ScaleRatio based on the scale of this Component's Transform.
	 	 * @param space: The space to be used in order to compute ScaleRatio. You can use local/world transform, canvas and window space.
		*/
		void RecalculateScaleRatio(UISpace space = UISpace::Window);
		

		
		virtual void SetContent(const String&);
		void SetTransform(const Transform& transform) override
		{
			RenderableComponent::SetTransform(transform);
			TransformDirtyFlag = static_cast<int>(GetTransform().AddDirtyFlag());
		}
		void SetMaterialInst(MaterialInstance&&);

		// Font+style setters
		void SetFont(SharedPtr<Font> font) { _Font = font; InvalidateCache(); }

		void SetFont(const String& fontPath);

		void SetFontStyle(FontStyle style) { _FontStyle = style; InvalidateCache(); }
		
		// 2D alignment setters
		void SetHorizontalAlignment(Alignment horizontal) {	_Alignment.SetHorizontal(horizontal); InvalidateCache(); }

		void SetVerticalAlignment(Alignment horizontal)	{ _Alignment.SetVertical(horizontal); InvalidateCache(); }

		void SetAlignment(Alignment2D alignment) { _Alignment = alignment; InvalidateCache(); }


		void Render(const SceneMatrixInfo& info, Shader* shader) override;


		void HandleEvent(const Event& ev) override;
		/**
		 * @brief Unstretch the text (make the scale uniform) based on this Component's Transform. The scale X and Y will be set to the smallest component of the two.
		 * @param space: The space to be used in order to compute ScaleRatio. You can use local/world transform, canvas and window space.
		*/
		virtual void Unstretch(UISpace space = UISpace::Window);
		virtual void InvalidateCache() const
		{
			LetterTransformsCache.clear();
		}

		// UI methods
		void GetEditorDescription(ComponentDescriptionBuilder) override;

		MaterialInstance GetDebugMatInst(ButtonMaterialType) override;

		unsigned int GetUIDepth() const override;

		// Serialization methods
		template <typename Archive> void Save(Archive& archive) const
		{
			String fontPath;
			if (_Font)
				fontPath = _Font->GetVariation(FontStyle::Regular)->GetPath();
			archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", _Alignment.GetHorizontal()), cereal::make_nvp("VAlignment", _Alignment.GetVertical()), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
		}
		template <typename Archive> void Load(Archive& archive)
		{
			String fontPath;
			Alignment alignmentH, alignmentV;
			archive(cereal::make_nvp("FontPath", fontPath), cereal::make_nvp("Content", Content), cereal::make_nvp("HAlignment", alignmentH), cereal::make_nvp("VAlignment", alignmentV), cereal::make_nvp("RenderableComponent", cereal::base_class<RenderableComponent>(this)));
			SetAlignment(Alignment2D(alignmentH, alignmentV));

			if (!fontPath.empty())
				_Font = EngineDataLoader::LoadFont(*GetGameHandle(), fontPath);
		}

	protected:
		void CheckTransformDirtiness() const
		{
			if (GetTransform().GetDirtyFlag(TransformDirtyFlag))
				InvalidateCache();
		}

	private:
		
		/**
		 * \brief Gets transforms for rendering this text's letters from cache. If cache is dirty, recomputes them.
		 * \return An unsafe reference to the vector containing cached letter transforms. When this TextComponent is modified, the returned reference becomes unsafe.
		 */
		const Vector<Transform>& GetLetterTransformsUnsafe();

		String Content;
		SharedPtr<Font> _Font;
		UniquePtr<MaterialInstance> TextMatInst;
		mutable Vector<Transform> LetterTransformsCache;


		/*Vertical and horizontal alignment. Alignment is in relation to Component::ComponentTransform::Position
		  Default alignment is Left and Center. This means that Component::ComponentTransform::Position marks the left-center point of the text.
		  CENTER alignment means that the TextComponent is centered around Component::ComponentTransform::Position.
		  RIGHT means that the text is moved during rendering so it ends at Component::ComponentTransform::Position and TOP that the next never goes above previous Component::ComponentTransform::Position::y.
		  Note that Component::ComponentTransform::Position is not always at the bottom-left corner of the text.
		  Use a function (that DOES NOT EXIST YET. TODO) to get the bottom-left corner of the text.
		*/
		Alignment2D _Alignment;					

		Vec2f MinSize, MaxSize;
		Vec2f ScaleRatio;

		FontStyle _FontStyle;
		int TransformDirtyFlag;
	};

	class ScrollingTextComponent : public TextComponent
	{
	public:
		ScrollingTextComponent(Actor&, Component* parentComp, const String& name = String(), const Transform& transform = Transform(), String content = String(), SharedPtr<Font> font = nullptr, Alignment2D = Alignment2D::LeftCenter());
		ScrollingTextComponent(Actor&, Component* parentComp, const String& name = String(), const Transform& transform = Transform(), String content = String(), String fontPath = String(), Alignment2D = Alignment2D::LeftCenter());

		bool IsScrollable() const override;

		void ScrollToLetter(unsigned int letterIndex);

		Boxf<Vec2f> GetBoundingBox(bool world = true) const override;	//Canvas space
		Alignment2D GetAlignment() const override;

		Transform GetCorrectedTransform(bool world = true);

		void Update(Time dt) override;
		void Render(const SceneMatrixInfo& info, Shader* shader) override;

		void InvalidateCache() const override
		{
			TextComponent::InvalidateCache();
			IsScrollableCacheDirty = true;
		}

	private:

		Time TimeSinceScrollReset, ScrollResetTime, ScrollCooldownTime;
		Interpolation ScrollingInterp;

		mutable bool IsScrollableCache, IsScrollableCacheDirty;
		
	};

	namespace TextUtil
	{
		float GetTextLength(const String& str, const Vec2f& textScale, const Font::Variation&);
		float GetTextHeight(const String& str, const Vec2f& textScale, const Font::Variation&);
		Vec2f GetTextMaxVerticalExtents(const String& str, const Vec2f& textScale, const Font::Variation&);

		Boxf<Vec2f> ComputeBBox(const String& str, const Transform& spaceTransform, const Font::Variation&, Alignment2D);

		Vec2f ComputeScale(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio = Vec2f(1.0f));
		Vec2f ComputeScaleRatio(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio = Vec2f(1.0f));

		Transform OwnedToSpaceTransform(UISpace space, const Transform& ownedTransform, const UICanvas* canvas, const WindowData* windowData);

		Vector<Transform> ComputeLetterTransforms(const String& content, Transform textTransform, Alignment2D, const Font::Variation&);

	}
}
GEE_POLYMORPHIC_SERIALIZABLE_COMPONENT(GEE::Component, GEE::TextComponent, GEE::Transform(), "", "")