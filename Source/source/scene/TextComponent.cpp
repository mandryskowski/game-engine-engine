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

	Boxf<Vec2f> TextComponent::GetBoundingBox(bool world)
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (GetCanvasPtr() && world)
			transform = GetCanvasPtr()->ToCanvasSpace(transform);

		float textLength = GetTextLength(world);

		Vec2f alignmentOffset((static_cast<float>(Alignment.first) - static_cast<float>(TextAlignment::CENTER)) * -textLength, (static_cast<float>(Alignment.second) - static_cast<float>(TextAlignment::CENTER)) * -transform.GetScale().y);

		return Boxf<Vec2f>(Vec2f(transform.GetPos()) + alignmentOffset, Vec2f(textLength, transform.GetScale().y));
	}

	float TextComponent::GetTextLength(bool world) const
	{
		Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
		if (CanvasPtr && world)
			transform = CanvasPtr->ToCanvasSpace(transform);

		Vec2f scale = transform.GetScale();

		float advancesSum = 0.0f;
		for (auto it : Content)
			advancesSum += UsedFont->GetCharacter(it).Advance;

		return advancesSum * scale.x;
	}

	std::vector<const Material*> TextComponent::GetMaterials() const
	{
		if (TextMatInst)
			return { &TextMatInst->GetMaterialRef() };

		return std::vector<const Material*>();
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
		float textLength = glm::max(GetTextLength(false) / GetTransform().GetScale().x, 0.001f);
		float scale = glm::min(MaxSize.y, MaxSize.x / textLength);

		GetTransform().SetScale(Vec2f(scale) * ScaleRatio);
	}

}