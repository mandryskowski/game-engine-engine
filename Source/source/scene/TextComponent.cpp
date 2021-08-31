#include <scene/TextComponent.h>
#include <assetload/FileLoader.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <UI/Font.h>
#include <UI/UICanvas.h>
#include <math/Transform.h>

#include <scene/Actor.h>
#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>

#include <input/InputDevicesStateRetriever.h>

namespace GEE
{
	TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, SharedPtr<Font> font, std::pair<TextAlignment, TextAlignment> alignment) :
		RenderableComponent(actor, parentComp, name, transform),
		UIComponent(actor, parentComp),
		UsedFont(font),
		TextMatInst(nullptr),
		Alignment(TextAlignment::LEFT, TextAlignment::BOTTOM)
	{
		Material& mat = *GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Default_Text_Material");
		//mat-SetColor(Vec4f(0.7f, 0.2f, 0.6f, 1.0f));

		TextMatInst = MakeUnique<MaterialInstance>(MaterialInstance(mat));
		SetAlignment(alignment);
		SetContent(content);
	}

	TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment) :
		TextComponent(actor, parentComp, name, transform, content, (!fontPath.empty()) ? (EngineDataLoader::LoadFont(*actor.GetScene().GetGameHandle(), fontPath)) : (actor.GetGameHandle()->GetDefaultFont()), alignment)
	{
	}

	TextComponent::TextComponent(TextComponent&& textComp) :
		RenderableComponent(std::move(textComp)),
		UIComponent(std::move(textComp)),
		Content(textComp.Content),
		UsedFont(textComp.UsedFont),
		TextMatInst(std::move(textComp.TextMatInst)),
		Alignment(textComp.Alignment)
	{
	}


	const std::string& TextComponent::GetContent() const
	{
		return Content;
	}

	Boxf<Vec2f> TextComponent::GetBoundingBox(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (GetCanvasPtr() && world)
			transform = GetCanvasPtr()->ToCanvasSpace(transform);

		float halfTextLength = GetTextLength(world) / 2.0f;

		Vec2f alignmentOffset((static_cast<float>(Alignment.first) - static_cast<float>(TextAlignment::CENTER)) * -halfTextLength, (static_cast<float>(Alignment.second) - static_cast<float>(TextAlignment::CENTER)) * -transform.GetScale().y);

		return Boxf<Vec2f>(Vec2f(transform.GetPos()) + alignmentOffset, Vec2f(halfTextLength, transform.GetScale().y));
	}

	float TextComponent::GetTextLength(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::GetTextLength(Content, transform.GetScale(), *UsedFont);
	}

	float TextComponent::GetTextHeight(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		return textUtil::GetTextHeight(Content, transform.GetScale(), *UsedFont);
	}

	std::vector<const Material*> TextComponent::GetMaterials() const
	{
		if (TextMatInst)
			return { &TextMatInst->GetMaterialRef() };

		return std::vector<const Material*>();
	}

	Font* TextComponent::GetUsedFont()
	{
		return UsedFont.get();
	}

	MaterialInstance* TextComponent::GetTextMatInst()
	{
		return TextMatInst.get();
	}

	std::pair<TextAlignment, TextAlignment> TextComponent::GetAlignment() const
	{
		return Alignment;
	}

	void TextComponent::SetContent(const std::string& content)
	{
		Content = content;
	}

	void TextComponent::SetMaterialInst(MaterialInstance&& matInst)
	{
		TextMatInst = MakeUnique<MaterialInstance>(std::move(matInst));
	}

	void TextComponent::Render(const RenderInfo& info, Shader* shader)
	{
		if (!UsedFont || GetHide())
			return;
		GameHandle->GetRenderEngineHandle()->RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), *UsedFont, Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().GetColor(), shader, false, Alignment);
		//	GameHandle->GetRenderEngineHandle()->RenderStaticMesh(RenderInfo(*GameHandle->GetRenderEngineHandle()->GetCurrentTbCollection()), MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD)), Transform(), GameHandle->GetRenderEngineHandle()->FindShader("Debug")));
		if (CanvasPtr)
			CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
	}

	void TextComponent::SetHorizontalAlignment(const TextAlignment horizontal)
	{
		Alignment.first = horizontal;
	}

	void TextComponent::SetVerticalAlignment(const TextAlignment horizontal)
	{
		Alignment.second = horizontal;
	}

	void TextComponent::SetAlignment(const TextAlignment horizontal, const TextAlignment vertical)
	{
		SetHorizontalAlignment(horizontal);
		SetVerticalAlignment(vertical);
	}

	void TextComponent::SetAlignment(const std::pair<TextAlignment, TextAlignment>& alignment)
	{
		Alignment = alignment;
	}

	void TextComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
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
					alignmentField.CreateChild<UIButtonActor>(hAlignmentsStr[j] + "Button", hAlignmentsStr[j], [this, j]() {this->SetHorizontalAlignment(static_cast<TextAlignment>(j)); }).GetTransform()->Move(Vec2f(static_cast<float>(j) * 2.0f, 0.0f));
				else
					alignmentField.CreateChild<UIButtonActor>(vAlignmentsStr[j] + "Button", vAlignmentsStr[j], [this, j]() {this->SetVerticalAlignment(static_cast<TextAlignment>(j)); }).GetTransform()->Move(Vec2f(static_cast<float>(j) * 2.0f, 0.0f));
		}
	}

	MaterialInstance TextComponent::LoadDebugMatInst(EditorIconState state)
	{
		LoadDebugRenderMaterial("GEE_Mat_Default_Debug_TextComponent", "EditorAssets/textcomponent_icon.png");
		return Component::LoadDebugMatInst(state);
	}

	unsigned int TextComponent::GetUIDepth() const
	{
		return GetElementDepth();
	}

	TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, SharedPtr<Font> font, std::pair<TextAlignment, TextAlignment> alignment) :
		TextComponent(actor, parentComp, name, transform, "", font, alignment),
		MaxSize(Vec2f(1.0f)),
		ScaleRatio(glm::max(Vec2f(1.0f), Vec2f(transform.GetScale().x / transform.GetScale().y, transform.GetScale().y / transform.GetScale().x)))
	{
		SetContent(content);
	}

	TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment) :
		TextComponent(actor, parentComp, name, transform, "", fontPath, alignment),
		MaxSize(Vec2f(1.0f)),
		ScaleRatio(glm::max(Vec2f(1.0f), Vec2f(transform.GetScale().x / transform.GetScale().y, transform.GetScale().y / transform.GetScale().x)))
	{
		SetContent(content);
	}

	TextConstantSizeComponent::TextConstantSizeComponent(TextConstantSizeComponent&& textComp) :
		TextComponent(std::move(textComp)),
		MaxSize(textComp.MaxSize),
		ScaleRatio(textComp.ScaleRatio)
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
		float textLength = glm::max((GetTextLength(false) / 2.0f) / GetTransform().GetScale().x, 0.001f);
		//float textHeight = glm::max((GetTextHeight(false) / 2.0f) / GetTransform().GetScale().y, 0.001f);
		float scale = glm::min(MaxSize.y/* / textHeight*/, MaxSize.x / textLength);

		GetTransform().SetScale(Vec2f(scale) * ScaleRatio);
	}

	void TextConstantSizeComponent::RecalculateScaleRatio(bool world)
	{
		Vec2f scale = ((world)
					? ((GetCanvasPtr()) ? (GetCanvasPtr()->ToCanvasSpace(GetTransform().GetWorldTransform())) : (GetTransform().GetWorldTransform()))
					: (GetTransform())).GetScale();
		ScaleRatio = Vec2f(glm::max(Vec2f(1.0f), Vec2f(scale.x / scale.y, scale.y / scale.x)));
	}

	void TextConstantSizeComponent::Unstretch(bool world)
	{
		Vec2f scale = ((world)
			? ((GetCanvasPtr()) ? (GetCanvasPtr()->ToCanvasSpace(GetTransform().GetWorldTransform())) : (GetTransform().GetWorldTransform()))
			: (GetTransform())).GetScale();

		Vec2f componentRatio(glm::min(Vec2f(scale.y / scale.x, scale.x / scale.y), Vec2f(1.0f)));

		ScaleRatio = componentRatio;
		UpdateSize();
		SetMaxSize(MaxSize / componentRatio);
		//GetTransform().SetScale(Vec3f(Vec2f(GetTransform().GetScale()) * componentRatio, GetTransform().GetScale().z));
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const std::string& name, const Transform& transform, std::string content, SharedPtr<Font> font, std::pair<TextAlignment, TextAlignment> alignment):
		TextComponent(actorRef, parentComp, name, transform, content, font, alignment),
		MaxLength(2.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f)
	{
	}

	ScrollingTextComponent::ScrollingTextComponent(Actor& actorRef, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment):
		TextComponent(actorRef, parentComp, name, transform, content, fontPath, alignment),
		MaxLength(2.0f),
		TimeSinceScrollReset(0.0f),
		ScrollResetTime(5.0f),
		ScrollCooldownTime(1.0f)
	{
	}

	Boxf<Vec2f> ScrollingTextComponent::GetBoundingBox(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (GetCanvasPtr() && world)
			transform = GetCanvasPtr()->ToCanvasSpace(transform);

		return Boxf<Vec2f>(transform.GetPos(), transform.GetScale());
	}


	void ScrollingTextComponent::Update(float deltaTime)
	{
		TimeSinceScrollReset += deltaTime;
		if (TimeSinceScrollReset > ScrollResetTime + ScrollCooldownTime)
			TimeSinceScrollReset -= (ScrollResetTime + ScrollCooldownTime) + ScrollCooldownTime;	// Two cooldowns: one after the text has been fully scrolled and one before scrolling. That's why we substract two cooldowns
	}

	void ScrollingTextComponent::Render(const RenderInfo& infoBeforeChange, Shader* shader)
	{
		if (!GetUsedFont() || GetHide())
			return;

		// Copy info
		Transform renderTransform = GetTransform().GetWorldTransform();
		RenderInfo info = infoBeforeChange;

		// Set view matrix
		float scroll = (glm::max(glm::min(TimeSinceScrollReset, ScrollResetTime), 0.0f) / ScrollResetTime);
		if (CanvasPtr) scroll *= CanvasPtr->GetCanvasT()->GetScale().x;
		const float scrollLength = (GetTextLength(true) - Vec4f(((CanvasPtr) ? (CanvasPtr->ToCanvasSpace(renderTransform)) : (renderTransform)).GetMatrix() * Vec4f(2.0f, 0.0f, 0.0f, 0.0f)).x);
		if (scrollLength <= 0.0f)
		{
			TextComponent::Render(info, shader);
			return;
		}
		const float posX = scroll * scrollLength;
		const Mat4f scrollMat = Transform(Vec2f(posX + renderTransform.GetScale().x, 0.0f), Vec2f(1.0f)).GetViewMatrix();

		info.view *= scrollMat;
		info.CalculateVP();


		// Set viewport
		glEnable(GL_SCISSOR_TEST);
		auto parentWorldT = GetTransform()./*GetParentTransform()->*/GetWorldTransform();
		if (CanvasPtr) parentWorldT = CanvasPtr->FromCanvasSpace(CanvasPtr->GetViewT().GetInverse() * CanvasPtr->ToCanvasSpace(parentWorldT));
		auto viewport = NDCViewport(parentWorldT.GetPos() - parentWorldT.GetScale(), parentWorldT.GetScale());
		viewport.ToPxViewport(GameHandle->GetGameSettings()->WindowSize).SetScissor();

		// Render
		GameHandle->GetRenderEngineHandle()->RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), *GetUsedFont(), GetContent(), renderTransform, GetTextMatInst()->GetMaterialRef().GetColor(), shader, false, std::pair<TextAlignment, TextAlignment>(TextAlignment::LEFT, GetAlignment().second));

		// Clean up after everything
		if (CanvasPtr)
			CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
		Viewport(Vec2u(0), GameHandle->GetGameSettings()->WindowSize).SetOpenGLState();
		glDisable(GL_SCISSOR_TEST);
	}

	Transform ScrollingTextComponent::GetParentWorldTransform() const
	{
		if (GetTransform().GetParentTransform())
			return GetTransform().GetParentTransform()->GetWorldTransform();
		return Transform();
	}

	float textUtil::GetTextLength(const std::string& str, const Vec2f& textScale, const Font& font)
	{
		std::vector<float> advancesSums = { 0.0f };
		for (auto it : str)
		{
			if (it == '\n')
			{
				advancesSums.push_back(0.0f);
				continue;
			}
			advancesSums.back() += font.GetCharacter(it).Advance;
		}

		return *std::max_element(advancesSums.begin(), advancesSums.end()) * textScale.x * 2.0f;
	}

	float textUtil::GetTextHeight(const std::string& str, const Vec2f& textScale, const Font& font)
	{
		float textHeight = 0.0f;
		float thisLineMaxHeight = 0.0f;
		for (auto it : str)
		{
			thisLineMaxHeight = 1.0f;
			if (it == '\n')
			{
				textHeight += thisLineMaxHeight;
				thisLineMaxHeight = 0.0f;
				continue;
			}

			if (float size = font.GetCharacter(it).Size.y; size > thisLineMaxHeight)
				thisLineMaxHeight = size;
		}

		return (textHeight + thisLineMaxHeight) * textScale.y;
	}

}