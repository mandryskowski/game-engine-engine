#include "ButtonActor.h"
#include "HUDModelComponent.h"
#include "MeshSystem.h"
#include "GameSettings.h"

ButtonActor::ButtonActor(GameScene* scene, const std::string& name):
	Actor(scene, name),
	IsMouseHovering(false)
{

}

void ButtonActor::HandleInputs(GLFWwindow* window)
{
	double cursorX, cursorY;
	glfwGetCursorPos(window, &cursorX, &cursorY);

//	glm::vec2 viewportSize((float)GameHandle->GetGameSettings()->ViewportData.z, (float)GameHandle->GetGameSettings()->ViewportData.w);
	glm::vec2 windowSize((glm::vec2)GameHandle->GetGameSettings()->WindowSize);
	glm::vec2 windowBottomLeft(0.0f);// ((glm::vec2)GameHandle->GetGameSettings()->ViewportData);
	//viewportSize = windowSize;

	glm::vec2 cursorNDC = (glm::vec2(cursorX, windowSize.y - cursorY) - windowBottomLeft) / windowSize * 2.0f - 1.0f;

	if (CollisionTests::AlignedRectContainsPoint(RootComponent->GetTransform(), cursorNDC))
		OnHover();
	else
		OnUnhover();
}

void ButtonActor::OnStart()
{
	Material* buttonMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle");
	if (!buttonMaterial)
	{
		buttonMaterial = new Material("GEE_Button_Idle");
		buttonMaterial->SetColor(glm::vec4(0.25f, 0.5f, 0.75f, 1.0f));
		buttonMaterial->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(buttonMaterial);

		buttonMaterial = new Material("GEE_Button_Hover");
		buttonMaterial->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		buttonMaterial->SetRenderShaderName("Forward_NoLight");
		GameHandle->GetRenderEngineHandle()->AddMaterial(buttonMaterial);
	}

	buttonMaterial = GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle");
	
	MeshSystem::MeshTree* tree = GameHandle->GetRenderEngineHandle()->FindMeshTree("GEE_ButtonTemplate");
	if (!tree)
	{
		tree = GameHandle->GetRenderEngineHandle()->CreateMeshTree("GEE_ButtonTemplate");
		MeshSystem::MeshNode* meshNode = tree->GetRoot().AddChild<MeshSystem::MeshNode>("ButtonQuad");
		meshNode->AddMesh(GameHandle->GetRenderEngineHandle()->GetBasicShapeMesh(EngineBasicShape::QUAD));
		meshNode->GetMesh(0)->SetMaterial(buttonMaterial);
	}

	RootComponent->AddComponent(ModelComponent::Of(ModelComponent(Scene, *dynamic_cast<MeshSystem::MeshNode*>(tree->FindNode("ButtonQuad")), "ButtonModel")).get());
}

void ButtonActor::OnHover()
{
	IsMouseHovering = true;

	RootComponent->GetComponent<ModelComponent>("ButtonModel")->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Hover"));
}

void ButtonActor::OnUnhover()
{
	IsMouseHovering = false;

	RootComponent->GetComponent<ModelComponent>("ButtonModel")->OverrideInstancesMaterial(GameHandle->GetRenderEngineHandle()->FindMaterial("GEE_Button_Idle"));
}

bool CollisionTests::AlignedRectContainsPoint(const Transform& rect, const glm::vec2& point)
{
	glm::vec2 rectLeftBottom = rect.PositionRef - rect.ScaleRef / 2.0f;
	return point == glm::min(glm::max(rectLeftBottom, point), rectLeftBottom + glm::vec2(rect.ScaleRef));
}
