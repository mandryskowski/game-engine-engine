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
	TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, const std::string& content, SharedPtr<Font> font, Alignment2D alignment) :
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		_Font(font),
		_FontStyle(FontStyle::Regular),
		TextMatInst(nullptr),
		_Alignment(alignment)
	{
		auto mat = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Default_Text_Material");
		//mat-SetColor(Vec4f(0.7f, 0.2f, 0.6f, 1.0f));

		TextMatInst = MakeUnique<MaterialInstance>(mat);
		SetAlignment(alignment);
		SetContent(content);
	}

	TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, const std::string& content, const std::string& fontPath, Alignment2D alignment) :
		TextComponent(actor, parentComp, name, transform, content, (!fontPath.empty()) ? (EngineDataLoader::LoadFont(*actor.GetScene().GetGameHandle(), fontPath)) : (actor.GetGameHandle()->GetDefaultFont()), alignment)
	{
	}

	TextComponent::TextComponent(TextComponent&& textComp) :
		RenderableComponent(std::move(textComp)),
		UIComponent(std::move(textComp)),
		Content(textComp.Content),
		_Font(textComp._Font),
		TextMatInst(std::move(textComp.TextMatInst)),
		_Alignment(textComp._Alignment)
	{
	}


	const std::string& TextComponent::GetContent() const
	{
		return Content;
	}

	Boxf<Vec2f> TextComponent::GetBoundingBox(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::ComputeBBox(Content, transform, *GetFontVariation(), _Alignment);
	}

	float TextComponent::GetTextLength(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::GetTextLength(Content, transform.GetScale(), *GetFontVariation());
	}

	float TextComponent::GetTextHeight(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::GetTextHeight(Content, transform.GetScale(), *GetFontVariation());
	}

	Vec2f TextComponent::GetTextMaxVerticalExtents(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::GetTextMaxVerticalExtents(Content, transform.GetScale(), *GetFontVariation());
	}

	std::vector<const Material*> TextComponent::GetMaterials() const
	{
		if (TextMatInst)
			return { &TextMatInst->GetMaterialRef() };

		return std::vector<const Material*>();
	}

	Font* TextComponent::GetFont()
	{
		return _Font.get();
	}

	FontStyle TextComponent::GetFontStyle() const
	{
		return _FontStyle;
	}

	Font::Variation* TextComponent::GetFontVariation()
	{
		return _Font->GetVariation(_FontStyle);
	}

	const Font::Variation* TextComponent::GetFontVariation() const
	{
		return _Font->GetVariation(_FontStyle);
	}

	MaterialInstance* TextComponent::GetTextMatInst()
	{
		return TextMatInst.get();
	}

	Alignment2D TextComponent::GetAlignment() const
	{
		return _Alignment;
	}

	void TextComponent::SetContent(const std::string& content)
	{
		Content = content;
	}

	void TextComponent::SetMaterialInst(MaterialInstance&& matInst)
	{
		TextMatInst = MakeUnique<MaterialInstance>(std::move(matInst));
	}

	void TextComponent::SetFont(const std::string& fontPath)
	{
		SetFont(EngineDataLoader::LoadFont(*GameHandle, fontPath));
	}

	void TextComponent::Render(const SceneMatrixInfo& info, Shader* shader)
	{
		if (!_Font || GetHide() || IsBeingKilled())
			return;

		if (!Material::ShaderInfo(MaterialShaderHint::None).MatchesRequiredInfo(info.GetRequiredShaderInfo()) && shader && shader->GetName() != "TextShader")
			return;

		TextRenderer(*GameHandle->GetRenderEngineHandle()).RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), *_Font->GetVariation(_FontStyle), Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().GetColor(), shader, false, _Alignment);
		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
	}

	void TextComponent::SetHorizontalAlignment(Alignment horizontal)
	{
		_Alignment.SetHorizontal(horizontal);
	}

	void TextComponent::SetVerticalAlignment(Alignment horizontal)
	{
		_Alignment.SetVertical(horizontal);
	}

	void TextComponent::SetAlignment(Alignment2D alignment)
	{
		_Alignment = alignment;
	}

	void TextComponent::Unstretch(UISpace space)
	{
		GetTransform().SetScale(GetTransform().GetScale2D() * textUtil::ComputeScaleRatio(space, ComponentTransform, GetCanvasPtr(), ((Scene.GetUIData()) ? (&Scene.GetUIData()->GetWindowData()) : (nullptr))));
	}

	void TextComponent::GetEditorDescription(ComponentDescriptionBuilder descBuilder)
	{
		RenderableComponent::GetEditorDescription(descBuilder);

		UIInputBoxActor& contentInputBox = descBuilder.AddField("Content").CreateChild<UIInputBoxActor>("ContentInputBox");
		contentInputBox.SetOnInputFunc([this](const std::string& content) { SetContent(content); }, [this]() ->std::string { return GetContent(); });

		for (int i = 0; i < 2; i++)
		{
			UICanvasField& alignmentField = descBuilder.AddField(std::string((i == 0) ? ("Horizontal") : ("Vertical")) + " alignment");
			const std::string hAlignmentsStr[] = { "Left", "Center", "Right" };
			const std::string vAlignmentsStr[] = { "Bottom", "Center", "Top" };

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

	TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, SharedPtr<Font> font, Alignment2D alignment) :
		TextComponent(actor, parentComp, name, transform, content, font, alignment),
		MaxSize(Vec2f(1.0f)),
		ScaleRatio(Vec2f(1.0f)),
		PreviouslyAppliedScaleRatio(Vec2f(1.0f))
	{
		Unstretch();
	}

	TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, Alignment2D alignment) :
		TextComponent(actor, parentComp, name, transform, content, fontPath, alignment),
		MaxSize(Vec2f(1.0f)),
		ScaleRatio(Vec2f(1.0f)),
		PreviouslyAppliedScaleRatio(Vec2f(1.0f))
	{
		Unstretch();
	}

	TextConstantSizeComponent::TextConstantSizeComponent(TextConstantSizeComponent&& textComp) :
		TextComponent(std::move(textComp)),
		MaxSize(textComp.MaxSize),
		ScaleRatio(textComp.ScaleRatio),
		PreviouslyAppliedScaleRatio(textComp.PreviouslyAppliedScaleRatio)
	{
	}

	void TextConstantSizeComponent::SetMaxSize(const Vec2f& maxSize)
	{
		if (MaxSize == maxSize)
			return;
		MaxSize = maxSize;
		UpdateSize();
	}

	void TextConstantSizeComponent::SetContent(const std::string& content)
	{
		if (GetContent() == content)
			return;

		TextComponent::SetContent(content);
		UpdateSize();
	}

	void TextConstantSizeComponent::UpdateSize()
	{
		float textLength = glm::max((GetTextLength(false) / 2.0f) / (GetTransform().GetScale().x / PreviouslyAppliedScaleRatio.x), 0.001f);
		float textHeight = glm::max((GetTextHeight(false)) / (GetTransform().GetScale().y / PreviouslyAppliedScaleRatio.y), 0.001f);
		float scale = glm::min(MaxSize.x / textLength, MaxSize.y / textHeight);
		
		//GetTransform().SetScale(Vec2f(scale) * ScaleRatio);
		PreviouslyAppliedScaleRatio = Vec3f(1.0f);
	}

	Boxf<Vec2f> TextConstantSizeComponent::GetBoundingBox(bool world) const
	{
		Transform transform = GetTransformCorrectedForSize(world);
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::ComputeBBox(GetContent(), transform, *GetFontVariation(), GetAlignment());
	}

	void TextConstantSizeComponent::RecalculateScaleRatio(UISpace space)
	{
		ScaleRatio = textUtil::ComputeScaleRatio(space, ComponentTransform, GetCanvasPtr(), ((Scene.GetUIData()) ? (&Scene.GetUIData()->GetWindowData()) : (nullptr)), Vec3f(1.0f));
	}

	void TextConstantSizeComponent::Unstretch(UISpace space)
	{
		RecalculateScaleRatio(space);
		UpdateSize();
	}

	Transform TextConstantSizeComponent::GetTransformCorrectedForSize(bool world) const
	{
		float textLength = glm::max(GetTextLength(false) / 2.0f, 0.001f);
		Vec2f verticalExtents = GetTextMaxVerticalExtents(false);
		float textHeight = glm::max(glm::max(glm::abs(verticalExtents.x), glm::abs(verticalExtents.y)), 0.001f) * 2.0f;
		Vec2f scaleRatioInverse = 1.0f / ScaleRatio;
		float scale = glm::min(scaleRatioInverse.x * MaxSize.x / textLength, scaleRatioInverse.y * MaxSize.y / textHeight);

		Transform t = GetTransform();
		t.SetScale(Vec2f(scale) * ScaleRatio * t.GetScale2D());
		return (world) ? (GetTransform().GetParentTransform()->GetWorldTransform() * t) : (t);
	}

	void TextConstantSizeComponent::HandleEvent(const Event& ev)
	{
		TextComponent::HandleEvent(ev);

		if (ev.GetType() == EventType::WindowResized)
			Unstretch();
	}

	void TextConstantSizeComponent::Render(const SceneMatrixInfo& info, Shader* shader)
	{
		if (!GetFont() || GetHide() || IsBeingKilled())
			return;

		if (!Material::ShaderInfo(MaterialShaderHint::None).MatchesRequiredInfo(info.GetRequiredShaderInfo()) && shader && shader->GetName() != "TextShader")
			return;

		TextRenderer(*GameHandle->GetRenderEngineHandle()).RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), *GetFont()->GetVariation(GetFontStyle()), GetContent(), GetTransformCorrectedForSize(true), GetTextMatInst()->GetMaterialRef().GetColor(), shader, false, GetAlignment());
		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const std::string& name, const Transform& transform, std::string content, SharedPtr<Font> font, Alignment2D alignment):
		TextComponent(actorRef, parentComp, name, transform, content, font, alignment),
		ScrollingInterp(0.0f, 5.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f)
	{
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, Alignment2D alignment):
		TextComponent(actorRef, parentComp, name, transform, content, fontPath, alignment),
		ScrollingInterp(0.0f, 5.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f)
	{
	}

	bool ScrollingTextComponent::IsScrollable() const
	{
		Transform parentRenderTransform = GetTransform().GetParentTransform()->GetWorldTransform();
		float scroll = ScrollingInterp.GetT();

		if (CanvasPtr) scroll *= CanvasPtr->GetCanvasT()->GetScale().x;
		const float scrollLength = (GetTextLength(true) - Vec4f(parentRenderTransform.GetMatrix() * Vec4f(2.0f, 0.0f, 0.0f, 0.0f)).x);

		return scrollLength > 0.0f;
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
		float scroll = ScrollingInterp.GetT();

		if (CanvasPtr) scroll *= CanvasPtr->GetCanvasT()->GetScale().x;
		const float scrollLength = (GetTextLength(world) - Vec4f(parentRenderTransform.GetMatrix() * Vec4f(2.0f, 0.0f, 0.0f, 0.0f)).x);
		//if (!world) std::cout << "!scroll length: " << scrollLength << '\n';
		//if (!world) std::cout << "!value of T: " << scroll << '\n';
		if (scrollLength <= 0.0f)
			return renderTransform;

		const float posX = scroll * scrollLength;


		if (GetContent() == "UIFix.json")
		{
			std::cout << "====Scrolling text: " << GetContent() << "====\n";
			std::cout << "@@@ PosX: " << posX << '\n';
			std::cout << "@@@ AlignmentH: " << (int)GetAlignment().GetHorizontal() << '\n';
			std::cout << "@@@ Render transform scale: " << renderTransform.GetScale2D() << '\n';
			std::cout << "@@@ Local length: " << GetTextLength(false) << "\n";
			std::cout << "@@@ World length: " << GetTextLength(true) << "\n";
			std::cout << "\n";
		}

		return Transform(-Vec2f(posX + parentRenderTransform.GetScale2D().x, 0.0f), Vec2f(1.0f)) * renderTransform;
	}


	void ScrollingTextComponent::Update(float deltaTime)
	{
		ScrollingInterp.UpdateT(deltaTime);
		if (ScrollingInterp.GetT() >= 1.0f)
			ScrollingInterp.Reset();
		//TimeSinceScrollReset += deltaTime;
		//if (TimeSinceScrollReset > ScrollResetTime + ScrollCooldownTime)
		//	TimeSinceScrollReset -= (ScrollResetTime + ScrollCooldownTime) + ScrollCooldownTime;	// Two cooldowns: one after the text has been fully scrolled and one before scrolling. That's why we substract two cooldowns
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
		//glEnable(GL_SCISSOR_TEST);
		auto parentWorldT = GetTransform().GetParentTransform()->GetWorldTransform();
		if (CanvasPtr) parentWorldT = CanvasPtr->FromCanvasSpace(CanvasPtr->GetViewT().GetInverse() * CanvasPtr->ToCanvasSpace(parentWorldT));
		auto viewport = NDCViewport(parentWorldT.GetPos() - parentWorldT.GetScale(), parentWorldT.GetScale());
		viewport.ToPxViewport(infoBeforeChange.GetTbCollection().GetSettings().Resolution).SetScissor();

		// Render - force left alignment if scrolling
		TextRenderer(*GameHandle->GetRenderEngineHandle()).RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info)) : (info), *GetFont(), GetContent(), scrolledT, GetTextMatInst()->GetMaterialRef().GetColor(), shader, false, Alignment::Left);

		// Clean up after everything
		if (CanvasPtr)
			CanvasPtr->UnbindForRender();
		Viewport(Vec2u(0), infoBeforeChange.GetTbCollection().GetSettings().Resolution).SetOpenGLState();
		glDisable(GL_SCISSOR_TEST);
	}

	float textUtil::GetTextLength(const std::string& str, const Vec2f& textScale, const Font::Variation& fontVariation)
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

	float textUtil::GetTextHeight(const std::string& str, const Vec2f& textScale, const Font::Variation& fontVariation)
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

	Vec2f textUtil::GetTextMaxVerticalExtents(const std::string& str, const Vec2f& textScale, const Font::Variation& fontVariation)
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

	Boxf<Vec2f> textUtil::ComputeBBox(const std::string& str, const Transform& spaceTransform, const Font::Variation& fontVariation, Alignment2D alignment)
	{
		float halfTextLength = GetTextLength(str, spaceTransform.GetScale2D(), fontVariation) / 2.0f;

		Vec2f alignmentOffset((static_cast<float>(alignment.GetHorizontal()) - static_cast<float>(Alignment::Center)) * -halfTextLength, (static_cast<float>(alignment.GetVertical()) - static_cast<float>(Alignment::Center)) * -spaceTransform.GetScale().y);

		return Boxf<Vec2f>(Vec2f(spaceTransform.GetPos()) + alignmentOffset, Vec2f(halfTextLength, GetTextHeight(str, spaceTransform.GetScale2D(), fontVariation)));
	}

	Vec2f textUtil::ComputeScale(UISpace space, const Transform& textTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio)
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
	Vec2f textUtil::ComputeScaleRatio(UISpace space, const Transform& ownedTextTransform, UICanvas* canvas, const WindowData* windowData, const Vec2f& optionalPreviouslyAppliedRatio)
	{
		return Math::GetRatioOfComponents(ComputeScale(space, ownedTextTransform, canvas, windowData, optionalPreviouslyAppliedRatio));
	}
	Transform textUtil::OwnedToSpaceTransform(UISpace space, const Transform& ownedTransform, const UICanvas* canvas, const WindowData* windowData)
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
}