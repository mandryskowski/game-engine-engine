#include <scene/TextComponent.h>
#include <assetload/FileLoader.h>
#include <rendering/RenderInfo.h>
#include <rendering/Material.h>
#include <UI/Font.h>
#include <UI/UICanvas.h>
#include <math/Transform.h>
#include <rendering/Mesh.h>
#include <iostream>

TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::shared_ptr<Font> font, std::pair<TextAlignment, TextAlignment> alignment):
	RenderableComponent(actor, parentComp, name, transform),
	UsedFont(font),
	TextMatInst(nullptr),
	Alignment(TextAlignment::LEFT, TextAlignment::BOTTOM)
{
	Material* mat = GameHandle->GetRenderEngineHandle()->AddMaterial(new Material("TextMaterial" + content));
	if (content == "big button")
		mat->SetColor(glm::vec4(0.7f, 0.2f, 0.6f, 1.0f));
	else
		mat->SetColor(glm::vec4(0.0f, 0.607057f, 0.663284f, 1.0f));
	TextMatInst = std::make_unique<MaterialInstance>(MaterialInstance(*mat));
	SetAlignment(alignment);
	SetContent(content);
}

#include <scene/Actor.h>
TextComponent::TextComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment) :
	TextComponent(actor, parentComp, name, transform, content, EngineDataLoader::LoadFont(*actor.GetScene().GetGameHandle(), fontPath), alignment)
{
}

TextComponent::TextComponent(TextComponent&& textComp) :
	RenderableComponent(std::move(textComp)),
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

Box2f TextComponent::GetBoundingBox(bool world) 
{
	Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
	if (CanvasPtr && world)
		transform = CanvasPtr->ToCanvasSpace(transform);

	float textLength = GetTextLength(world);

	glm::vec2 alignmentOffset((static_cast<float>(Alignment.first) - static_cast<float>(TextAlignment::CENTER)) * -textLength, (static_cast<float>(Alignment.second) - static_cast<float>(TextAlignment::CENTER)) * -transform.ScaleRef.y);

	return Box2f(glm::vec2(transform.PositionRef) + alignmentOffset, glm::vec2(textLength, transform.ScaleRef.y));
}

float TextComponent::GetTextLength(bool world) const
{
	Transform transform = (world) ? (GetTransform().GetWorldTransform()) : (GetTransform());
	if (CanvasPtr && world)
		transform = CanvasPtr->ToCanvasSpace(transform);

	glm::vec2 scale = transform.ScaleRef;

	float advancesSum = 0.0f;
	for (auto it : Content)
		advancesSum += UsedFont->GetCharacter(it).Advance;

	return advancesSum * scale.x;
}

void TextComponent::SetContent(const std::string& content)
{
	Content = content;
}

void TextComponent::Render(const RenderInfo& info, Shader* shader)
{
	if (!UsedFont)
		return;
	GameHandle->GetRenderEngineHandle()->RenderText((CanvasPtr) ? (CanvasPtr->BindForRender(info, GameHandle->GetGameSettings()->WindowSize)) : (info), *UsedFont, Content, GetTransform().GetWorldTransform(), TextMatInst->GetMaterialRef().Color, shader, false, Alignment);
//	GameHandle->GetRenderEngineHandle()->RenderStaticMesh(RenderInfo(*GameHandle->GetRenderEngineHandle()->GetCurrentTbCollection()), MeshInstance(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD)), Transform(), GameHandle->GetRenderEngineHandle()->FindShader("Debug")));
	if (CanvasPtr)
		CanvasPtr->UnbindForRender(GameHandle->GetGameSettings()->WindowSize);
}

void TextComponent::SetAlignment(const TextAlignment horizontal, const TextAlignment vertical)
{
	SetAlignment(std::pair<TextAlignment, TextAlignment>(horizontal, vertical));
}

void TextComponent::SetAlignment(const std::pair<TextAlignment, TextAlignment>& alignment)
{
	Alignment = alignment;
}

#include <UI/UICanvasActor.h>
#include <UI/UICanvasField.h>
#include <scene/UIInputBoxActor.h>
void TextComponent::GetEditorDescription(EditorDescriptionBuilder descBuilder)
{
	RenderableComponent::GetEditorDescription(descBuilder);

	UIInputBoxActor& contentInputBox = descBuilder.AddField("Content").CreateChild<UIInputBoxActor>("ContentInputBox");
	contentInputBox.SetOnInputFunc([this](const std::string& content) { SetContent(content); }, [this]() ->std::string { return GetContent(); });
}

TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::shared_ptr<Font> font, std::pair<TextAlignment, TextAlignment> alignment):
	TextComponent(actor, parentComp, name, transform, "", font, alignment),
	MaxSize(glm::vec2(1.0f)),
	ScaleRatio(glm::max(glm::vec2(1.0f), glm::vec2(transform.ScaleRef.x / transform.ScaleRef.y, transform.ScaleRef.y / transform.ScaleRef.x)))
{
	SetContent(content);
}

#include <scene/Actor.h>
TextConstantSizeComponent::TextConstantSizeComponent(Actor& actor, Component* parentComp, const std::string& name, const Transform& transform, std::string content, std::string fontPath, std::pair<TextAlignment, TextAlignment> alignment):
	TextConstantSizeComponent(actor, parentComp, name, transform, content, EngineDataLoader::LoadFont(*actor.GetScene().GetGameHandle(), fontPath), alignment)
{
}

TextConstantSizeComponent::TextConstantSizeComponent(TextConstantSizeComponent&& textComp):
	TextComponent(std::move(textComp)),
	MaxSize(textComp.MaxSize),
	ScaleRatio(textComp.ScaleRatio)
{
}

void TextConstantSizeComponent::SetMaxSize(const glm::vec2& maxSize)
{
	MaxSize = maxSize;
}

void TextConstantSizeComponent::SetContent(const std::string& content)
{
	if (GetContent() == content)
		return;

	TextComponent::SetContent(content);
	
	float textLength = glm::max(GetTextLength(false) / GetTransform().ScaleRef.x, 0.001f);
	float scale = glm::min(MaxSize.y, MaxSize.x / textLength);

	std::cout << "Length: " << textLength << " finalscale: " << scale << " scalex: " << GetTransform().ScaleRef.x << '\n';
	printVector(ScaleRatio, "Scale ratio");

	GetTransform().SetScale(glm::vec2(scale) * ScaleRatio);
}
