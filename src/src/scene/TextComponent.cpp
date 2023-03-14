#include <scene/TextComponent.h>
#include <game/GameScene.h>
#include <assetload/FileLoader.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <rendering/RenderToolbox.h>
#include <UI/Font.h>
#include <UI/UICanvas.h>
#include <math/Transform.h>
#include <rendering/Renderer.h>

#include <scene/Actor.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>

#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	Transform test(UICanvas* canvas, Transform& t) {
		return canvas->FromCanvasSpace(canvas->GetViewT().GetInverse() * canvas->ToCanvasSpace(t.GetWorldTransform()));
	}
	TextComponent::TextComponent(Actor& actor, Component* parentComp, const String& name, const Transform& transform, const String& content, SharedPtr<Font> font, Alignment2D alignment) :
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		_Font(font),
		_FontStyle(FontStyle::Regular),
		TextMatInst(nullptr),
		_Alignment(alignment),
		MinSize(Vec2f(0.0f)),
		MaxSize(Vec2f(std::numeric_limits<float>::max())),
		ScaleRatio(Vec2f(1.0f)),
		PreRatioScale(transform.GetScale2D()),
		PostRatioScale(transform.GetScale2D()),
		TransformDirtyFlag(0),
		ScaleDirtyFlag(0)
	{
		auto mat = GetGameHandle()->GetRenderEngineHandle()->FindMaterial("GEE_Default_Text_Material");

		TextMatInst = MakeUnique<MaterialInstance>(mat);
		SetAlignment(alignment);
		SetContent(content);
	}

	TextComponent::TextComponent(Actor& actor, Component* parentComp, const String& name, const Transform& transform, const String& content, const String& fontPath, Alignment2D alignment) :
		TextComponent(actor, parentComp, name, transform, content, (!fontPath.empty()) ? (EngineDataLoader::LoadFont(*actor.GetScene().GetGameHandle(), fontPath)) : (actor.GetGameHandle()->GetDefaultFont()), alignment)
	{
	}

	TextComponent::TextComponent(TextComponent&& textComp) :
		RenderableComponent(std::move(textComp)),
		UIComponent(std::move(textComp)),
		Content(textComp.Content),
		_Font(textComp._Font),
		TextMatInst(std::move(textComp.TextMatInst)),
		_Alignment(textComp._Alignment),
		TransformDirtyFlag(textComp.TransformDirtyFlag)
	{
	}


	String TextComponent::GetContent() const
	{
		return Content;
	}

	Boxf<Vec2f> TextComponent::GetBoundingBox(bool world) const
	{
		return TextUtil::ComputeBBox(GetContent(), GetTransformCorrectedForSize((world) ? (UISpace::Canvas) : (UISpace::Local)), *GetFontVariation(), GetAlignment());
	}

	float TextComponent::GetTextLength(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return TextUtil::GetTextLength(Content, transform.GetScale(), *GetFontVariation());
	}

	float TextComponent::GetTextHeight(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return TextUtil::GetTextHeight(Content, transform.GetScale(), *GetFontVariation());
	}

	Vec2f TextComponent::GetTextMaxVerticalExtents(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return TextUtil::GetTextMaxVerticalExtents(Content, transform.GetScale(), *GetFontVariation());
	}

	std::vector<const Material*> TextComponent::GetMaterials() const
	{
		if (TextMatInst)
			return { &TextMatInst->GetMaterialRef() };

		return std::vector<const Material*>();
	}

	Transform TextComponent::GetTransformCorrectedForSize(UISpace space) const
	{
		std::function<Transform(const Transform&)> convertSpaceFunc = [this, space](const Transform& t)
		{
			switch (space)
			{
			case UISpace::Local: return t;
			case UISpace::World: 
			{
				Transform copy = t;
				return GetTransform().GetParentTransform()->GetWorldTransform() * copy;
			}
			case UISpace::Canvas:
			{
				auto worldT = GetTransform().GetParentTransform()->GetWorldTransform() * t;
				return (CanvasPtr) ? (CanvasPtr->ToCanvasSpace(worldT)) : (worldT);
			}
			default: GEE_CORE_ASSERT(true);
			}
		};

		if (GetMaxSize() == Vec2f(std::numeric_limits<float>::max()))
			return convertSpaceFunc(GetTransform());

		float textLength = glm::max(GetTextLength(false) / 2.0f, 0.001f);
		Vec2f verticalExtents = GetTextMaxVerticalExtents(false);
		float textHeight = glm::max(glm::max(glm::abs(verticalExtents.x), glm::abs(verticalExtents.y)), 0.001f) * 2.0f;
		Vec2f scaleRatioInverse = Vec2f(1.0f);// / ScaleRatio;
		float scale = glm::min(scaleRatioInverse.x * GetMaxSize().x / textLength, scaleRatioInverse.y * GetMaxSize().y / textHeight);


		Transform t = GetTransform();
		t.SetScale(Vec2f(scale) /*ScaleRatio*/ * t.GetScale2D());
			

		return convertSpaceFunc(t);
	}

	void TextComponent::RecalculateScaleRatio(UISpace space)
	{
		const auto windowData = GetScene().GetUIData()->GetWindowData();
		CheckTransformDirtiness();

		if (GetTransform().GetScale2D() == PostRatioScale)
			GetTransform().SetScale(PreRatioScale);
		else
		{
			std::cout << "Unfo, " << GetTransform().GetScale2D() << " and " << PostRatioScale << '\n';
			PreRatioScale = GetTransform().GetScale2D();
		}

		const auto newScaleRatio = TextUtil::ComputeScaleRatio(space, GetTransform(), GetCanvasPtr(),
		                                         ((GetScene().GetUIData()) ? (&windowData) : (nullptr)), Vec3f(1.0f));
		if (newScaleRatio != ScaleRatio)
		{
			ScaleRatio = newScaleRatio;
			InvalidateCache();
		}
	}

	void TextComponent::SetContent(const String& content)
	{
		if (Content == content)
			return;

		Content = content;
		InvalidateCache();
	}

	void TextComponent::SetMaterialInst(MaterialInstance&& matInst)
	{
		TextMatInst = MakeUnique<MaterialInstance>(std::move(matInst));
	}

	void TextComponent::SetFont(const String& fontPath)
	{
		SetFont(EngineDataLoader::LoadFont(*GetGameHandle(), fontPath));
	}

	void TextComponent::Render(const SceneMatrixInfo& info, Shader* shader)
	{
		if (!_Font || GetHide() || IsBeingKilled())
			return;

		if (!Material::ShaderInfo(MaterialShaderHint::None).MatchesRequiredInfo(info.GetRequiredShaderInfo()) && shader && shader->GetName() != "TextShader")
			return;

		CheckTransformDirtiness();
		TextRenderer(*GetGameHandle()->GetRenderEngineHandle()).RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), *_Font->GetVariation(_FontStyle), Content, GetLetterTransformsUnsafe(), TextMatInst->GetMaterialRef().GetColor(), shader, false, _Alignment);
		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
	}

	void TextComponent::HandleEvent(const Event& ev)
	{
		RenderableComponent::HandleEvent(ev);

		if (ev.GetType() == EventType::WindowResized || ev.GetType() == EventType::CanvasViewChanged)
			Unstretch(UISpace::World);

		if (ev.GetType() == EventType::KeyPressed && dynamic_cast<const KeyEvent&>(ev).GetKeyCode() == Key::P)
			Unstretch();
	}

	void TextComponent::Unstretch(UISpace space)
	{
		auto windowData = GetScene().GetUIData()->GetWindowData();

		if (GetName() == "GEE_Engine_Title_Text") {
			std::cout << GetContent() << " Scale before: " << GetTransform().GetScale2D() << '\n';
			std::cout << GetContent() << " PreRatio: " << PreRatioScale << '\n';
		}
		CheckTransformDirtiness();

		RecalculateScaleRatio(space);

		auto scale = PreRatioScale * ScaleRatio;


		// We'd rather avoid calling SetScale if the scale didn't change, because that would flag the
		// transform as dirty and lead to unnecessary (quite expensive) computation.
		//if (scale != GetTransform().GetScale2D())
		{
			GetTransform().SetScale(scale);
			PostRatioScale = GetTransform().GetScale2D();
		}
		InvalidateCache();
			
		if (GetName() == "GEE_Engine_Title_Text") {
			std::cout << GetContent() << " Scale post: " << scale << '\n';
			std::cout << GetContent() << " PostRatio: " << PostRatioScale << '\n';
			std::cout << GetContent() << " SR: " << ScaleRatio << '\n';
		}
	}

	void TextComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		RenderableComponent::GetEditorDescription(descBuilder);

		UIInputBoxActor& contentInputBox = descBuilder.AddField("Content").CreateChild<UIInputBoxActor>("ContentInputBox");
		contentInputBox.SetOnInputFunc([this](const String& content) { SetContent(content); }, [this]() ->String { return GetContent(); });

		for (int i = 0; i < 2; i++)
		{
			UICanvasField& alignmentField = descBuilder.AddField(String((i == 0) ? ("Horizontal") : ("Vertical")) + " alignment");
			const String hAlignmentsStr[] = { "Left", "Center", "Right" };
			const String vAlignmentsStr[] = { "Bottom", "Center", "Top" };

			for (int j = 0; j < 3; j++)
				if (i == 0)
					alignmentField.CreateChild<UIButtonActor>(hAlignmentsStr[j] + "Button", hAlignmentsStr[j], [this, j]() {this->SetHorizontalAlignment(static_cast<Alignment>(j)); }).GetTransform()->Move(Vec2f(static_cast<float>(j) * 2.0f, 0.0f));
				else
					alignmentField.CreateChild<UIButtonActor>(vAlignmentsStr[j] + "Button", vAlignmentsStr[j], [this, j]() {this->SetVerticalAlignment(static_cast<Alignment>(j)); }).GetTransform()->Move(Vec2f(static_cast<float>(j) * 2.0f, 0.0f));
		}
	}

	MaterialInstance TextComponent::GetDebugMatInst(ButtonMaterialType type)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_TextComponent", "Assets/Editor/textcomponent_icon.png");
		return Component::GetDebugMatInst(type);
	}

	unsigned int TextComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}

	const Vector<Transform>& TextComponent::GetLetterTransformsUnsafe()
	{
		if (!LetterTransformsCache.empty())
			return LetterTransformsCache;

		ScrollingTextComponent* dupa = dynamic_cast<ScrollingTextComponent*>(this);
		dupa = nullptr;

		std::cout << "Generating " << GetContent() << ": " << GetTransform().GetScale2D() << '\n';

		return LetterTransformsCache = TextUtil::ComputeLetterTransforms(GetContent(),
		                                                          dupa ? dupa->GetCorrectedTransform(true) : GetTransformCorrectedForSize(UISpace::World),
		                                                          GetAlignment(), *GetFontVariation());
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const String& name, const Transform& transform, String content, SharedPtr<Font> font, Alignment2D alignment):
		TextComponent(actorRef, parentComp, name, transform, content, font, alignment),
		ScrollingInterp(0.0f, 5.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f),
		IsScrollableCache(false),
		IsScrollableCacheDirty(true)
	{
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const String& name, const Transform& transform, String content, String fontPath, Alignment2D alignment):
		TextComponent(actorRef, parentComp, name, transform, content, fontPath, alignment),
		ScrollingInterp(0.0f, 5.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f),
		IsScrollableCache(false),
		IsScrollableCacheDirty(true)
	{
	}


	bool ScrollingTextComponent::IsScrollable() const
	{
		CheckTransformDirtiness();
		if (IsScrollableCacheDirty)
		{
			Transform parentRenderTransform = GetTransform().GetParentTransform()->GetWorldTransform();
			if (CanvasPtr)
				parentRenderTransform = CanvasPtr->ToCanvasSpace(parentRenderTransform);

			const float scrollLength = GetTextLength(true) - parentRenderTransform.GetScale2D().x * 2.0f;

			IsScrollableCache = scrollLength > 0.0f;
			IsScrollableCacheDirty = false;
		}

		return IsScrollableCache;
	}

	void ScrollingTextComponent::ScrollToLetter(unsigned int letterIndex)
	{
		Transform transform = GetTransform().GetWorldTransform();
		Transform parentRenderTransform = GetTransform().GetParentTransform()->GetWorldTransform();

		if (CanvasPtr)
		{
			transform = CanvasPtr->ToCanvasSpace(transform);
			parentRenderTransform = CanvasPtr->ToCanvasSpace(parentRenderTransform);
		}

		const float fullLength = GetTextLength(true) - parentRenderTransform.GetScale2D().x * 2.0f;
		const float partialLength = TextUtil::GetTextLength(GetContent().substr(0, letterIndex), transform.GetScale2D(), *GetFontVariation()) - parentRenderTransform.GetScale2D().x * 2.0f;

		ScrollingInterp.SetT(glm::clamp(partialLength / fullLength, 0.0f, 1.0f));
	}

	Boxf<Vec2f> ScrollingTextComponent::GetBoundingBox(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (GetCanvasPtr() && world)
			transform = GetCanvasPtr()->ToCanvasSpace(transform);

		return Boxf<Vec2f>(transform.GetPos(), transform.GetScale());
	}

	Alignment2D ScrollingTextComponent::GetAlignment() const
	{
		if (IsScrollable())
			return Alignment::Left;

		return TextComponent::GetAlignment();
	}

	Transform ScrollingTextComponent::GetCorrectedTransform(bool world)
	{
	
		Transform renderTransform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		Transform parentRenderTransform = (world) ? (GetTransform().GetParentTransform()->GetWorldTransform()) : (Transform());

		// Generate scroll matrix
		auto scroll = ScrollingInterp.GetT();
		auto scrollLength = 0.0f;

		if (CanvasPtr && world)
			scrollLength = CanvasPtr->ScaleFromCanvasSpace(Vec2f((GetTextLength(world) - Vec4f(CanvasPtr->ToCanvasSpace(parentRenderTransform).GetScale2D().x * 2.0f)))).x;
		else
			scrollLength = GetTextLength(world) - parentRenderTransform.GetScale2D().x * 2.0f;
		
		if (scrollLength <= 0.0f)
			return renderTransform;

		const auto posX = scroll * scrollLength;

		return Transform(-Vec2f(posX + parentRenderTransform.GetScale2D().x, 0.0f), Vec2f(1.0f)) * renderTransform;
	}


	void ScrollingTextComponent::Update(Time dt)
	{
		return;
		ScrollingInterp.UpdateT(0.0f);
		if (ScrollingInterp.GetT() >= 1.0f)
			ScrollingInterp.Reset();
	}

	void ScrollingTextComponent::Render(const SceneMatrixInfo& infoBeforeChange, Shader* shader)
	{
		if (!GetFont() || GetHide() || IsBeingKilled())
			return;

		// Copy info
		SceneMatrixInfo info = infoBeforeChange;

		// Check if can be scrolled. If not, render as normal text.
		if (!IsScrollable())
		{
			TextComponent::Render(info, shader);
			return;
		}

		// Otherwise generate scrolled transform
		const Transform scrolledT = GetCorrectedTransform(true);

		// Set viewport
		glEnable(GL_SCISSOR_TEST);
		auto parentWorldT = GetTransform().GetParentTransform()->GetWorldTransform();
		if (CanvasPtr) parentWorldT = CanvasPtr->FromCanvasSpace(CanvasPtr->GetViewT().GetInverse() * CanvasPtr->ToCanvasSpace(parentWorldT));
		auto viewport = NDCViewport(parentWorldT.GetPos() - parentWorldT.GetScale(), parentWorldT.GetScale());
		viewport.ToPxViewport(infoBeforeChange.GetTbCollection().GetVideoSettings().Resolution).SetScissor();

		// Render - force left alignment if scrolling
		TextRenderer(*GetGameHandle()->GetRenderEngineHandle()).RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), *GetFontVariation(), GetContent(), scrolledT, GetTextMatInst()->GetMaterialRef().GetColor(), shader, false, Alignment::Left);

		// Clean up after everything
		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
		Viewport(Vec2u(0), infoBeforeChange.GetTbCollection().GetVideoSettings().Resolution).SetOpenGLState();
		glDisable(GL_SCISSOR_TEST);
	}

	float TextUtil::GetTextLength(const String& str, const Vec2f& textScale, const Font::Variation& fontVariation)
	{
		std::vector<float> advancesSums = { 0.0f };
		for (auto it : str)
		{
			if (it == '\n')
			{
				advancesSums.push_back(0.0f);
				continue;
			}
			advancesSums.back() += fontVariation.GetCharacter(it).Advance;
		}

		return *std::max_element(advancesSums.begin(), advancesSums.end()) * textScale.x * 2.0f;
	}

	float TextUtil::GetTextHeight(const String& str, const Vec2f& textScale, const Font::Variation& fontVariation)
	{
		float textHeight = 0.0f;
		float thisLineMaxHeight = 0.0f;
		for (auto it : str)
		{
			if (it == '\n')
			{
				textHeight += thisLineMaxHeight;
				thisLineMaxHeight = 0.0f;
				continue;
			}

			if (float size = fontVariation.GetCharacter(it).Size.y; size > thisLineMaxHeight)
				thisLineMaxHeight = size;
		}

		return (textHeight + thisLineMaxHeight) * textScale.y;
	}

	Vec2f TextUtil::GetTextMaxVerticalExtents(const String& str, const Vec2f& textScale, const Font::Variation& fontVariation)
	{
		Vec2f extents(std::numeric_limits<float>::max(), std::numeric_limits<float>::min());
		for (auto it : str)
		{
			if (it == '\n')
			{
				continue;
			}

			const auto& c = fontVariation.GetCharacter(it);
			if (float max = c.Bearing.y; max > extents.y)
				extents.y = max;
			if (float min = c.Size.y - c.Bearing.y; min < extents.x)
				extents.x = min;
		}

		return extents * textScale.y;
	}

	Boxf<Vec2f> TextUtil::ComputeBBox(const String& str, const Transform& spaceTransform, const Font::Variation& fontVariation, Alignment2D alignment)
	{
		float halfTextLength = GetTextLength(str, spaceTransform.GetScale2D(), fontVariation) / 2.0f;

		Vec2f alignmentOffset((static_cast<float>(alignment.GetHorizontal()) - static_cast<float>(Alignment::Center)) * -halfTextLength, (static_cast<float>(alignment.GetVertical()) - static_cast<float>(Alignment::Center)) * -spaceTransform.GetScale().y);

		return Boxf<Vec2f>(Vec2f(spaceTransform.GetPos()) + alignmentOffset, Vec2f(halfTextLength, GetTextHeight(str, spaceTransform.GetScale2D(), fontVariation)));
	}

	Vec2f TextUtil::ComputeScale(UISpace space, const Transform& textTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio)
	{
		Vec2f scale(1.0f);

		if (space == UISpace::Window && windowData)
			scale /= Math::GetRatioOfComponents(windowData->GetWindowSize());
		scale /= optionalPreviouslyAppliedRatio; 

		switch (space)
		{
		case UISpace::Local:	scale *= textTransform.GetScale2D(); break;
		case UISpace::World:	scale *= textTransform.GetWorldTransform().GetScale2D(); break;

		case UISpace::Canvas:
		case UISpace::Window:
		{
			if (canvas)
				scale *= canvas->GetCanvasT()->GetWorldTransform().GetScale2D() * canvas->ScaleFromCanvasSpace((Mat2f)canvas->GetViewMatrix() * canvas->ToCanvasSpace(textTransform.GetWorldTransform()).GetScale2D());
			else
				scale *= textTransform.GetWorldTransform().GetScale2D();
			break;
		}
		default: break;
		}

		return scale;
	}
	Vec2f TextUtil::ComputeScaleRatio(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio)
	{
		return Math::GetRatioOfComponents(ComputeScale(space, ownedTextTransform, canvas, windowData, optionalPreviouslyAppliedRatio));
	}
	Transform TextUtil::OwnedToSpaceTransform(UISpace space, const Transform& ownedTransform, const UICanvas* canvas, const WindowData* windowData)
	{
			
		switch (space)
		{
		case UISpace::Local:	return ownedTransform;
		case UISpace::World:	return ownedTransform.GetWorldTransform();

		case UISpace::Canvas:
		{
			if (canvas)
				return canvas->FromCanvasSpace(canvas->GetViewT().GetInverse() * canvas->ToCanvasSpace(ownedTransform.GetWorldTransform()));
			else
				return ownedTransform.GetWorldTransform();
		}
		case UISpace::Window:
		{
			Transform windowT(Vec2f(0.0f), (windowData) ? (Math::GetRatioOfComponents(windowData->GetWindowSize())) : (Vec2f(0.0f)));
			if (canvas)
				return windowT * canvas->FromCanvasSpace(canvas->GetViewT().GetInverse() * canvas->ToCanvasSpace(ownedTransform.GetWorldTransform()));
			else
				return windowT * ownedTransform.GetWorldTransform();
		}
		default: return Transform();
		}
	}

	Vector<Transform> TextUtil::ComputeLetterTransforms(const String& content, Transform t, Alignment2D alignment, const Font::Variation& fontVariation)
	{
		Vector<Transform> outTransforms;

		Vec2f halfExtent = t.GetScale();
		float textHeight = t.GetScale().y;
		t.Move(Vec3f(0.0f, -t.GetScale().y * 2.0f + fontVariation.GetBaselineHeight() * t.GetScale().y * 2.0f, 0.0f));	//align to bottom (-t.ScaleRef.y), move down to the bottom of it (-t.ScaleRef.y), and then move up to baseline height (-t.ScaleRef.y * 2.0f + font.GetBaselineHeight() * halfExtent.y * 2.0f)

		// Align horizontally
		if (alignment.GetHorizontal() != Alignment::Left)
		{
			float textLength = TextUtil::GetTextLength(content, t.GetScale(), fontVariation);

			t.Move(t.GetRot() * Vec3f(-textLength / 2.0f * (static_cast<float>(alignment.GetHorizontal()) - static_cast<float>(Alignment::Left)), 0.0f, 0.0f));
		}

		// Align vertically
		if (alignment.GetVertical() != Alignment::Bottom)
			t.Move(t.GetRot() * Vec3f(0.0f, -textHeight * (static_cast<float>(alignment.GetVertical()) - static_cast<float>(Alignment::Bottom)), 0.0f));

		// Account for multiple lines if the text is not aligned to TOP. Each line (excluding the first one, that's why we cancel out the first line) moves the text up by its scale if it is aligned to Bottom and by 1/2 of its scale if it is aligned to CENTER.
		//if (auto firstLineBreak = content.find_first_of('\n'); alignment.GetVertical() != Alignment::Top && firstLineBreak != std::string::npos)
			// t.Move(t.GetRot() * Vec3f(0.0f, (textHeight - textUtil::GetTextHeight(content.substr(0, firstLineBreak), t.GetScale(), font)) / 2.0f * (static_cast<float>(Alignment::Top) - static_cast<float>(alignment.GetVertical())), 0.0f));

		const Mat3f& textRot = t.GetRotationMatrix();


		Transform initialT = t;

		for (int i = 0; i < static_cast<int>(content.length()); i++)
		{
			if (content[i] == '\n')
			{
				initialT.Move(Vec2f(0.0f, -halfExtent.y * 2.0f));
				t = initialT;
				continue;
			}

			const Character& c = fontVariation.GetCharacter(content[i]);

			if (content == "-3")
				std::cout << "%%%%%%: " << content[i] << ": " << t.GetScale() << '\n';
			t.Move(textRot * Vec3f(c.Bearing * halfExtent, 0.0f) * 2.0f);
			t.SetScale(halfExtent);

			outTransforms.push_back(t);

			t.Move(textRot * -Vec3f(c.Bearing * halfExtent, 0.0f) * 2.0f);

			t.Move(textRot * Vec3f(c.Advance * halfExtent.x, 0.0f, 0.0f) * 2.0f);
		}

		return outTransforms;
	}
}
